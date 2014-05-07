/*
 *  Copyright (C) 2004-2014, Eric Lund, Jon Gettler
 *  http://www.mvpmc.org/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/**
 * \file connection.c
 * Functions to handle creating connections to a MythTV backend and
 * interacting with those connections.  
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <cmyth_local.h>

static char * cmyth_conn_get_setting_unlocked(cmyth_conn_t conn, const char* hostname, const char* setting);

typedef struct {
	unsigned int version;
	char token[14]; // up to 13 chars used in v74 + the terminating NULL character
} myth_protomap_t;

static myth_protomap_t protomap[] = {
	{62, "78B5631E"},
	{63, "3875641D"},
	{64, "8675309J"},
	{65, "D2BB94C2"},
	{66, "0C0FFEE0"},
	{67, "0G0G0G0"},
	{68, "90094EAD"},
	{69, "63835135"},
	{70, "53153836"},
	{71, "05e82186"},
	{72, "D78EFD6F"},
	{73, "D7FE8D6F"},
	{74, "SingingPotato"},
	{75, "SweetRock"},
	{76, "FireWilde"},
	{77, "WindMark"},
	{78, "IceBurns"},
	{79, "BasaltGiant"},
	{80, "TaDah!"},
	{0, ""}
};

#define VERSION_CACHE_SIZE	8

struct version_cache_s {
	char *host;
	unsigned int version;
};

static struct version_cache_s version_cache[VERSION_CACHE_SIZE];

#ifdef _MSC_VER
/*
 * shameless rip from:
 * http://memset.wordpress.com/2010/10/09/inet_ntop-for-win32/
 */
static const char *
inet_ntop(int af, const void* src, char* dst, int cnt)
{
	struct sockaddr_in srcaddr;

	memset(&srcaddr, 0, sizeof(struct sockaddr_in));
	memcpy(&(srcaddr.sin_addr), src, sizeof(srcaddr.sin_addr));

	srcaddr.sin_family = af;
	if (WSAAddressToString((struct sockaddr*) &srcaddr,
			       sizeof(struct sockaddr_in), 0, dst,
			       (LPDWORD) &cnt) != 0) {
		return NULL;
	}

	return dst;
}
#endif /* _MSC_VER */

static unsigned int
get_host_version(char *host)
{
	int i;

	for (i=0; i<VERSION_CACHE_SIZE; i++) {
		if ((version_cache[i].host != NULL) &&
		    (strcmp(host, version_cache[i].host) == 0)) {
			return version_cache[i].version;
		}
	}

	/*
	 * Start the protocol negotiation by offering the highest version
	 * that libcmyth supports.
	 */
	return 77;
}

static void
set_host_version(char *host, unsigned int version)
{
	int i;

	for (i=0; i<VERSION_CACHE_SIZE; i++) {
		if ((version_cache[i].host != NULL) &&
		    (strcmp(host, version_cache[i].host) == 0)) {
			version_cache[i].version = version;
			return;
		}
	}

	for (i=0; i<VERSION_CACHE_SIZE; i++) {
		if (version_cache[i].host == NULL) {
			version_cache[i].host = strdup(host);
			version_cache[i].version = version;
			return;
		}
	}

	/* Evict a host at random */
#if defined(HAS_ARC4RANDOM)
	i = arc4random_uniform(VERSION_CACHE_SIZE);
#else
	i = rand() % VERSION_CACHE_SIZE;
#endif

	if (version_cache[i].host) {
		free(version_cache[i].host);
	}
	version_cache[i].host = strdup(host);
	version_cache[i].version = version;
}

/*
 * cmyth_conn_destroy(cmyth_conn_t conn)
 * 
 * Scope: PRIVATE (static)
 *
 * Description
 *
 * Tear down and release storage associated with a connection.  This
 * should only be called by cmyth_conn_release().  All others should
 * call ref_release() to release a connection.
 *
 * Return Value:
 *
 * None.
 */
static void
cmyth_conn_destroy(cmyth_conn_t conn)
{
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s {\n", __FUNCTION__);
	if (!conn) {
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s } !\n", __FUNCTION__);
		return;
	}
	if (conn->conn_buf) {
		free(conn->conn_buf);
	}
	if (conn->conn_fd >= 0) {
		cmyth_dbg(CMYTH_DBG_PROTO,
			  "%s: shutdown and close connection fd = %d\n",
			  __FUNCTION__, conn->conn_fd);
		shutdown(conn->conn_fd, SHUT_RDWR);
		closesocket(conn->conn_fd);
	}
	if (conn->conn_server) {
		ref_release(conn->conn_server);
		conn->conn_server = NULL;
	}
	pthread_mutex_destroy(&conn->conn_mutex);
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s }\n", __FUNCTION__);
}

/*
 * cmyth_conn_create(void)
 * 
 * Scope: PRIVATE (static)
 *
 * Description
 *
 * Allocate and initialize a cmyth_conn_t structure.  This should only
 * be called by cmyth_connect(), which establishes a connection.
 *
 * Return Value:
 *
 * Success: A non-NULL cmyth_conn_t (this type is a pointer)
 *
 * Failure: A NULL cmyth_conn_t
 */
static cmyth_conn_t
cmyth_conn_create(void)
{
	cmyth_conn_t ret = ref_alloc(sizeof(*ret));

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s {\n", __FUNCTION__);
	if (!ret) {
			cmyth_dbg(CMYTH_DBG_DEBUG, "%s }!\n", __FUNCTION__);
		return NULL;
	}
	ref_set_destroy(ret, (ref_destroy_t)cmyth_conn_destroy);

	ret->conn_fd = -1;
	ret->conn_buf = NULL;
	ret->conn_len = 0;
	ret->conn_buflen = 0;
	ret->conn_pos = 0;
	ret->conn_hang = 0;
	ret->conn_port = 0;
	ret->conn_server = NULL;
	cmyth_dbg(CMYTH_DBG_DEBUG, "%s }\n", __FUNCTION__);
	return ret;
}

/*
 * cmyth_connect(char *server, unsigned short port, unsigned buflen)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Create a connection to the port specified by 'port' on the
 * server named 'server'.  This creates a data structure called a
 * cmyth_conn_t which contains the file descriptor for a socket, a
 * buffer for reading from the socket, and information used to manage
 * offsets in that buffer.  The buffer length is specified in 'buflen'.
 *
 * The returned connection has a single reference.  The connection
 * will be shut down and closed when the last reference is released
 * using ref_release().
 *
 * Return Value:
 *
 * Success: A non-NULL cmyth_conn_t (this type is a pointer)
 *
 * Failure: A NULL cmyth_conn_t
 */
static char my_hostname[128];
static volatile cmyth_socket_t my_fd;

static void
sighandler(int sig)
{
	/*
	 * XXX: This is not thread safe...
	 */
	closesocket(my_fd);
	my_fd = -1;
}

static cmyth_conn_t
cmyth_connect(char *server, unsigned short port, unsigned buflen,
	      int tcp_rcvbuf)
{
	cmyth_conn_t ret = NULL;
	struct sockaddr_storage addr;
	unsigned char *buf = NULL;
	cmyth_socket_t fd;
#ifndef _MSC_VER
	void (*old_sighandler)(int);
	int old_alarm;
#endif
	int temp;
	socklen_t size;
	struct addrinfo hints;
	struct addrinfo *res;
	struct addrinfo *cur;
	char dest[INET6_ADDRSTRLEN];

	/*
	 * First try to establish the connection with the server.
	 * If this fails, we are going no further.
	 */
	memset(&addr, 0, sizeof(addr));
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	if (getaddrinfo(server, NULL, &hints, &res)) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cannot resolve hostname '%s'\n",
			  __FUNCTION__, server);
		return NULL;
	}
	for (cur = res; cur; cur = cur->ai_next) {
		if (cur->ai_family == AF_INET) {
			((struct sockaddr_in *)cur->ai_addr)->sin_port =
				htons(port);
			inet_ntop(AF_INET,
				  &((struct sockaddr_in *)cur->ai_addr)->sin_addr,
				  dest, sizeof(dest));
			memcpy(&addr, cur->ai_addr,
                               sizeof(struct sockaddr_in));
			temp = PF_INET;
			break;
		}
		if (cur->ai_family == AF_INET6) {
                        ((struct sockaddr_in6 *)cur->ai_addr)->sin6_port =
				htons(port);
			inet_ntop(AF_INET6,
				  &((struct sockaddr_in6 *)cur->ai_addr)->sin6_addr,
				  dest, sizeof(dest));
			memcpy(&addr, cur->ai_addr,
                               sizeof(struct sockaddr_in6));
			temp = PF_INET6;
			break;
		}
	}
	freeaddrinfo(res);
	if (!cur) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no AF_INET address for '%s'\n",
			  __FUNCTION__, server);
		return NULL;
	}

	fd = socket(temp, SOCK_STREAM, 0);
	if (fd < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cannot create socket (%d)\n",
			  __FUNCTION__, errno);
		return NULL;
	}

	/*
	 * Set tcp receive buffer size.
	 * On all myth protocol sockets this should be 4kb, otherwise we
	 * risk the connection hanging.
	 * For playback sockets the default of 43689 seems best, a buffer
	 * of only 4kb causes stuttering during playback.
	 */
	if (tcp_rcvbuf == 0)
		tcp_rcvbuf = 4096;

	temp = tcp_rcvbuf;
	size = sizeof(temp);
	setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void*)&temp, size);

	if(getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void*)&temp, &size)) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: could not get rcvbuf from socket(%d)\n",
			  __FUNCTION__, errno);
		temp = tcp_rcvbuf;
	}
	tcp_rcvbuf = temp;

	cmyth_dbg(CMYTH_DBG_PROTO, "%s: connecting to %s fd = %d\n",
		  __FUNCTION__, dest, fd);
#ifndef _MSC_VER
	old_sighandler = signal(SIGALRM, sighandler);
	old_alarm = alarm(5);
#endif
	my_fd = fd;
	if (connect(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: connect failed on port %d to '%s' (%d)\n",
			  __FUNCTION__, port, server, errno);
		closesocket(fd);
#ifndef _MSC_VER
		signal(SIGALRM, old_sighandler);
		alarm(old_alarm);
#endif
		return NULL;
	}
	my_fd = -1;
#ifndef _MSC_VER
	signal(SIGALRM, old_sighandler);
	alarm(old_alarm);
#endif

	if ((my_hostname[0] == '\0') &&
	    (gethostname(my_hostname, sizeof(my_hostname)) < 0)) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: gethostname failed (%d)\n",
			  __FUNCTION__, errno);
		goto shut;
	}

	/*
	 * Okay, we are connected. Now is a good time to allocate some
	 * resources.
	 */
	ret = cmyth_conn_create();
	if (!ret) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_conn_create() failed\n",
			  __FUNCTION__);
		goto shut;
	}
	buf = malloc(buflen * sizeof(unsigned char));
	if (!buf) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s:- malloc(%d) failed allocating buf\n",
			  __FUNCTION__, buflen * sizeof(unsigned char *));
		goto shut;
	}
	ret->conn_fd = fd;
	ret->conn_buflen = buflen;
	ret->conn_buf = buf;
	ret->conn_len = 0;
	ret->conn_pos = 0;
	ret->conn_version = get_host_version(server);
	ret->conn_tcp_rcvbuf = tcp_rcvbuf;
	ret->conn_server = ref_strdup(server);
	ret->conn_port = port;
	pthread_mutex_init(&ret->conn_mutex, NULL);
	return ret;

    shut:
	if (ret) {
		ref_release(ret);
	}
	cmyth_dbg(CMYTH_DBG_PROTO, "%s: error connecting to "
		  "%s, shutdown and close fd = %d\n",
		  __FUNCTION__, dest, fd);
	shutdown(fd, 2);
	closesocket(fd);
	return NULL;
}

static cmyth_conn_t
cmyth_conn_connect(char *server, unsigned short port, unsigned buflen,
		   int tcp_rcvbuf, int event)
{
	cmyth_conn_t conn;
	char announcement[256];
	unsigned long tmp_ver;
	int attempt = 0;

    top:
	conn = cmyth_connect(server, port, buflen, tcp_rcvbuf);
	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_connect(%s, %d, %d) failed\n",
			  __FUNCTION__, server, port, buflen);
		return NULL;
	}

	/*
	 * Find out what the Myth Protocol Version is for this connection.
	 * Loop around until we get agreement from the server.
	 */
	if (attempt == 0)
		tmp_ver = conn->conn_version;
	conn->conn_version = tmp_ver;

	/*
	 * Myth 0.23.1 (Myth 0.23 + fixes) introduced an out of sequence protocol version number (23056)
	 * due to the next protocol version number having already been bumped in trunk.
	 *
	 * http://www.mythtv.org/wiki/Myth_Protocol
	 */
	if (tmp_ver >= 62 && tmp_ver != 23056) { // Treat protocol version number 23056 the same as protocol 56
		myth_protomap_t *map = protomap;
		while (map->version != 0 && map->version != tmp_ver)
			map++;
		if (map->version == 0) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: failed to connect with any version\n",
				  __FUNCTION__);
			goto shut;
		}
		snprintf(announcement, sizeof(announcement),
			 "MYTH_PROTO_VERSION %ld %s",
			 conn->conn_version, map->token);
	} else {
		snprintf(announcement, sizeof(announcement),
			 "MYTH_PROTO_VERSION %ld", conn->conn_version);
	}
	if (cmyth_send_message(conn, announcement) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message('%s') failed\n",
			  __FUNCTION__, announcement);
		goto shut;
	}
	if (cmyth_rcv_version(conn, &tmp_ver) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_version() failed\n",
			  __FUNCTION__);
		goto shut;
	}
	cmyth_dbg(CMYTH_DBG_ERROR,
		  "%s: asked for version %ld, got version %ld\n",
		  __FUNCTION__, conn->conn_version, tmp_ver);
	if (conn->conn_version != tmp_ver) {
		if (attempt == 1) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: failed to connect with any version\n",
				  __FUNCTION__);
			goto shut;
		}
		attempt = 1;
		ref_release(conn);
		goto top;
	}
	cmyth_dbg(CMYTH_DBG_PROTO, "%s: agreed on Version %ld protocol\n",
		  __FUNCTION__, conn->conn_version);

	set_host_version(server, conn->conn_version);

	/*
	 * Generate a unique hostname for event connections, since the server
	 * will not send the same event multiple times to the same host.
	 */
	if (event) {
		char buf[128];
		snprintf(buf, sizeof(buf), "%s_%d_%p", my_hostname,
			 getpid(), conn);
		snprintf(announcement, sizeof(announcement),
			 "ANN Playback %s %d", buf, event);
	} else {
		snprintf(announcement, sizeof(announcement),
			 "ANN Playback %s 0", my_hostname);
	}
	if (cmyth_send_message(conn, announcement) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message('%s') failed\n",
			  __FUNCTION__, announcement);
		goto shut;
	}
	if (cmyth_rcv_okay(conn) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_rcv_okay() failed\n",
			  __FUNCTION__);
		goto shut;
	}

	/*
	 * All of the downstream code in libcmyth assumes a monotonically increasing version number.
	 * This was not the case for Myth 0.23.1 (0.23 + fixes) where protocol version number 23056
	 * was used since 57 had already been used in trunk.
	 *
	 * Convert from protocol version number 23056 to version number 56 so subsequent code within
	 * libcmyth uses the same logic for the 23056 protocol as would be used for protocol version 56.
	 */
	if (conn->conn_version == 23056) {
		conn->conn_version = 56;
	}

	return conn;

    shut:
	ref_release(conn);
	return NULL;
}

/*
 * cmyth_conn_connect_ctrl(char *server, unsigned short port, unsigned buflen)
 *
 * Scope: PUBLIC
 *
 * Description:
 *
 * Create a connection for use as a control connection within the
 * MythTV protocol.  Return a pointer to the newly created connection.
 * The connection is returned held, and may be released using
 * ref_release().
 *
 * Return Value:
 *
 * Success: Non-NULL cmyth_conn_t (this is a pointer type)
 *
 * Failure: NULL cmyth_conn_t
 */
cmyth_conn_t
cmyth_conn_connect_ctrl(char *server, unsigned short port, unsigned buflen,
			int tcp_rcvbuf)
{
	cmyth_conn_t ret;

	cmyth_dbg(CMYTH_DBG_PROTO, "%s: connecting control connection\n",
		  __FUNCTION__);
	ret = cmyth_conn_connect(server, port, buflen, tcp_rcvbuf, 0);
	cmyth_dbg(CMYTH_DBG_PROTO,
		  "%s: done connecting control connection ret = %p\n",
		  __FUNCTION__, ret);
	return ret;
}

cmyth_conn_t
cmyth_conn_reconnect(cmyth_conn_t conn)
{
	return cmyth_conn_connect_ctrl(conn->conn_server, conn->conn_port,
				       conn->conn_buflen,
				       conn->conn_tcp_rcvbuf);
}

cmyth_conn_t
cmyth_conn_connect_event(char *server, unsigned short port, unsigned buflen,
			 int tcp_rcvbuf)
{
	cmyth_conn_t ret;
	cmyth_dbg(CMYTH_DBG_PROTO, "%s: connecting event channel connection\n",
		  __FUNCTION__);
	ret = cmyth_conn_connect(server, port, buflen, tcp_rcvbuf, 1);
	cmyth_dbg(CMYTH_DBG_PROTO,
		  "%s: done connecting event channel connection ret = %p\n",
		  __FUNCTION__, ret);
	return ret;
}

static cmyth_file_t
cmyth_conn_connect_pathname(cmyth_proginfo_t prog,  cmyth_conn_t control,
			    unsigned buflen, int tcp_rcvbuf, char *pathname)
{
	cmyth_conn_t conn = NULL;
	char *announcement = NULL;
	char *myth_host = NULL;
	char reply[16];
	int err = 0;
	int count = 0;
	int r;
	int ann_size = sizeof("ANN FileTransfer []:[][]:[]");
	cmyth_file_t ret = NULL;

	if (!control) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: control is NULL\n",
			  __FUNCTION__);
		goto shut;
	}
	if (!prog) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: prog is NULL\n", __FUNCTION__);
		goto shut;
	}
	if (!prog->proginfo_host) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: prog host is NULL\n",
			  __FUNCTION__);
		goto shut;
	}
	if (!pathname) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: prog has no pathname in it\n",
			  __FUNCTION__);
		goto shut;
	}
	ret = cmyth_file_create(control);
	if (!ret) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_file_create() failed\n",
			  __FUNCTION__);
		goto shut;
	}
	cmyth_dbg(CMYTH_DBG_PROTO, "%s: connecting data connection\n",
		  __FUNCTION__);
	if (control->conn_version >= 17) {
		myth_host = cmyth_conn_get_setting_unlocked(control, prog->proginfo_host,
		                                   "BackendServerIP");
		if (myth_host && (strcmp(myth_host, "-1") == 0)) {
			ref_release(myth_host);
			myth_host = NULL;
		}
	}
	if (!myth_host) {
		cmyth_dbg(CMYTH_DBG_PROTO,
		          "%s: BackendServerIP setting not found. Using proginfo_host: %s\n",
		          __FUNCTION__, prog->proginfo_host);
		myth_host = ref_strdup(prog->proginfo_host);
	}
	conn = cmyth_connect(myth_host, prog->proginfo_port,
			     buflen, tcp_rcvbuf);
	cmyth_dbg(CMYTH_DBG_PROTO,
		  "%s: done connecting data connection, conn = %d\n",
		  __FUNCTION__, conn);
	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_connect(%s, %d, %d) failed\n",
			  __FUNCTION__,
			  myth_host, prog->proginfo_port, buflen);
		goto shut;
	}
	/*
	 * Explicitly set the conn version to the control version as cmyth_connect() doesn't and some of
	 * the cmyth_rcv_* functions expect it to be the same as the protocol version used by mythbackend.
	 */
	conn->conn_version = control->conn_version;

	ann_size += strlen(pathname) + strlen(my_hostname);
	announcement = malloc(ann_size);
	if (!announcement) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: malloc(%d) failed for announcement\n",
			  __FUNCTION__, ann_size);
		goto shut;
	}
	if (control->conn_version >= 44) {
		snprintf(announcement, ann_size,
			 "ANN FileTransfer %s[]:[]%s[]:[]",
			 my_hostname, pathname);
	}
	else {
		snprintf(announcement, ann_size,
			 "ANN FileTransfer %s[]:[]%s", my_hostname, pathname);
	}

	if (cmyth_send_message(conn, announcement) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message('%s') failed\n",
			  __FUNCTION__, announcement);
		goto shut;
	}
	ret->file_data = ref_hold(conn);
	count = cmyth_rcv_length(conn);
	if (count < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_length() failed (%d)\n",
			  __FUNCTION__, count);
		goto shut;
	}
	reply[sizeof(reply) - 1] = '\0';
	r = cmyth_rcv_string(conn, &err, reply, sizeof(reply) - 1, count); 
	if (err != 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_string() failed (%d)\n",
			  __FUNCTION__, err);
		goto shut;
	}
	if (strcmp(reply, "OK") != 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: reply ('%s') is not 'OK'\n",
			  __FUNCTION__, reply);
		goto shut;
	}
	count -= r;
	r = cmyth_rcv_long(conn, &err, &ret->file_id, count);
	if (err) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: (id) cmyth_rcv_long() failed (%d)\n",
			  __FUNCTION__, err);
		goto shut;
	}
	count -= r;
	r = cmyth_rcv_uint64(conn, &err, &ret->file_length, count);
	if (err) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: (length) cmyth_rcv_uint64() failed (%d)\n",
			  __FUNCTION__, err);
		goto shut;
	}
	count -= r;
	if (count != 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: %d leftover bytes\n",
			  __FUNCTION__, count);
	}
	free(announcement);
	ref_release(conn);
	ref_release(myth_host);
	return ret;

    shut:
	if (announcement) {
		free(announcement);
	}
	ref_release(ret);
	ref_release(conn);
	ref_release(myth_host);
	return NULL;
}

/*
 * cmyth_conn_connect_file(char *server, unsigned short port, unsigned buflen
 *                         cmyth_proginfo_t prog)
 *
 * Scope: PUBLIC
 *
 * Description:
 *
 * Create a file structure containing a data connection for use
 * transfering a file within the MythTV protocol.  Return a pointer to
 * the newly created file structure.  The connection in the file
 * structure is returned held as is the file structure itself.  The
 * connection will be released when the file structure is released.
 * The file structure can be released using ref_release().
 *
 * Return Value:
 *
 * Success: Non-NULL cmyth_file_t (this is a pointer type)
 *
 * Failure: NULL cmyth_file_t
 */
cmyth_file_t
cmyth_conn_connect_file(cmyth_proginfo_t prog,  cmyth_conn_t control,
			unsigned buflen, int tcp_rcvbuf)
{
	return cmyth_conn_connect_pathname(prog, control, buflen, tcp_rcvbuf,
					   prog->proginfo_pathname);
}

/*
 * cmyth_conn_connect_thumbnail(char *server, unsigned short port,
 *                              unsigned buflen
 *                              cmyth_proginfo_t prog)
 *
 * Scope: PUBLIC
 *
 * Description:
 *
 * Create a file structure containing a data connection for use
 * transfering a thumbnail image within the MythTV protocol.  Return a pointer
 * to the newly created file structure.  The connection in the file
 * structure is returned held as is the file structure itself.  The
 * connection will be released when the file structure is released.
 * The file structure can be released using ref_release().
 *
 * Note that the filesize of the PNG thumbnail is unknown, so clients must
 * simply read until they run out of data.
 *
 * Return Value:
 *
 * Success: Non-NULL cmyth_file_t (this is a pointer type)
 *
 * Failure: NULL cmyth_file_t
 */
cmyth_file_t
cmyth_conn_connect_thumbnail(cmyth_proginfo_t prog,  cmyth_conn_t control,
			     unsigned buflen, int tcp_rcvbuf)
{
	char pathname[256];

	if (!prog->proginfo_pathname) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: prog has no pathname in it\n",
			  __FUNCTION__);
		return NULL;
	}

	snprintf(pathname, sizeof(pathname), "%s.png", prog->proginfo_pathname);

	return cmyth_conn_connect_pathname(prog, control, buflen, tcp_rcvbuf,
					   pathname);
}

/*
 * cmyth_conn_connect_ring(char *server, unsigned short port, unsigned buflen
 *                         cmyth_recorder_t rec)
 *
 * Scope: PUBLIC
 *
 * Description:
 *
 * Create a new ring buffer connection for use transferring live-tv
 * using the MythTV protocol.  Return a pointer to the newly created
 * ring buffer connection.  The ring buffer connection is returned
 * held, and may be released using ref_release().
 *
 * Return Value:
 *
 * Success: Non-NULL cmyth_conn_t (this is a pointer type)
 *
 * Failure: NULL cmyth_conn_t
 */
int
cmyth_conn_connect_ring(cmyth_recorder_t rec, unsigned buflen, int tcp_rcvbuf)
{
	cmyth_conn_t conn;
	char *announcement;
	int ann_size = sizeof("ANN RingBuffer  ");
	char *server;
	unsigned short port;

	if (!rec) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: rec is NULL\n", __FUNCTION__);
		return -1;
	}

	server = rec->rec_server;
	port = rec->rec_port;

	cmyth_dbg(CMYTH_DBG_PROTO, "%s: connecting ringbuffer\n",
		  __FUNCTION__);
	conn = cmyth_connect(server, port, buflen, tcp_rcvbuf);
	cmyth_dbg(CMYTH_DBG_PROTO,
		  "%s: connecting ringbuffer, conn = %p\n",
		  __FUNCTION__, conn);
	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_connect(%s, %d, %d) failed\n",
			  __FUNCTION__, server, port, buflen);
		return -1;
	}

	ann_size += CMYTH_LONG_LEN + strlen(my_hostname);
	announcement = malloc(ann_size);
	if (!announcement) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: malloc(%d) failed for announcement\n",
			  __FUNCTION__, ann_size);
		goto shut;
	}
	snprintf(announcement, ann_size,
		 "ANN RingBuffer %s %d", my_hostname, rec->rec_id);
	if (cmyth_send_message(conn, announcement) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message('%s') failed\n",
			  __FUNCTION__, announcement);
		free(announcement);
		goto shut;
	}
	free(announcement);
	if (cmyth_rcv_okay(conn) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_rcv_okay() failed\n",
			  __FUNCTION__);
		goto shut;
	}

        rec->rec_ring->conn_data = conn;
	return 0;

    shut:
	ref_release(conn);
	return -1;
}

int
cmyth_conn_connect_recorder(cmyth_recorder_t rec, unsigned buflen,
			    int tcp_rcvbuf)
{
	cmyth_conn_t conn;
	char *server;
	unsigned short port;

	if (!rec) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: rec is NULL\n", __FUNCTION__);
		return -1;
	}

	server = rec->rec_server;
	port = rec->rec_port;

	cmyth_dbg(CMYTH_DBG_PROTO, "%s: connecting recorder control\n",
		  __FUNCTION__);
	conn = cmyth_conn_connect_ctrl(server, port, buflen, tcp_rcvbuf);
	cmyth_dbg(CMYTH_DBG_PROTO,
		  "%s: done connecting recorder control, conn = %p\n",
		  __FUNCTION__, conn);
	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_connect(%s, %d, %d) failed\n",
			  __FUNCTION__, server, port, buflen);
		return -1;
	}

	if (rec->rec_conn)
		ref_release(rec->rec_conn);
	rec->rec_conn = conn;

	return 0;
}

/*
 * cmyth_conn_check_block(cmyth_conn_t conn, unsigned long size)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Check whether a block has finished transfering from a backend
 * server. This non-blocking check looks for a response from the
 * server indicating that a block has been entirely sent to on a data
 * socket.
 *
 * Return Value:
 *
 * Success: 0 for not complete, 1 for complete
 *
 * Failure: -(errno)
 */
int
cmyth_conn_check_block(cmyth_conn_t conn, unsigned long size)
{
	fd_set check;
	struct timeval timeout;
	int length;
	int err = 0;
	unsigned long sent;

	if (!conn) {
		return -EINVAL;
	}
	timeout.tv_sec = timeout.tv_usec = 0;
	FD_ZERO(&check);
	FD_SET(conn->conn_fd, &check);
	if (select(conn->conn_fd + 1, &check, NULL, NULL, &timeout) < 0) {
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s: select failed (%d)\n",
			  __FUNCTION__, errno);
		return -(errno);
	}
	if (FD_ISSET(conn->conn_fd, &check)) {
		/*
		 * We have a bite, reel it in.
		 */
		length = cmyth_rcv_length(conn);
		if (length < 0) {
			return length;
		}
		cmyth_rcv_ulong(conn, &err, &sent, length);
		if (err) {
			return -err;
		}
		if (sent == size) {
			/*
			 * This block has been sent, return TRUE.
			 */
			cmyth_dbg(CMYTH_DBG_DEBUG,
				  "%s: block finished (%d bytes)\n",
				  __FUNCTION__, sent);
			return 1;
		} else {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: block finished short (%d bytes)\n",
				  __FUNCTION__, sent);
			return -ECANCELED;
		}
	}
	return 0;
}

/*
 * cmyth_conn_get_recorder_from_num(cmyth_conn_t control,
 *                                  cmyth_recorder_num_t num,
 *                                  cmyth_recorder_t rec)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Obtain a recorder from a connection by its recorder number.  The
 * recorder structure created by this describes how to set up a data
 * connection and play media streamed from a particular back-end recorder.
 *
 * This fills out the recorder structure specified by 'rec'.
 *
 * Return Value:
 *
 * Success: 0 for not complete, 1 for complete
 *
 * Failure: -(errno)
 */
cmyth_recorder_t
cmyth_conn_get_recorder_from_num(cmyth_conn_t conn, int id)
{
	int err, count;
	int r;
	long port;
	char msg[256];
	char reply[256];
	cmyth_recorder_t rec = NULL;

	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n",
			  __FUNCTION__);
		return NULL;
	}

	pthread_mutex_lock(&conn->conn_mutex);

	if ((rec=cmyth_recorder_create()) == NULL)
		goto fail;

	snprintf(msg, sizeof(msg), "GET_RECORDER_FROM_NUM[]:[]%d", id);

	if ((err = cmyth_send_message(conn, msg)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message() failed (%d)\n",
			  __FUNCTION__, err);
		goto fail;
	}

	count = cmyth_rcv_length(conn);
	if (count < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_length() failed (%d)\n",
			  __FUNCTION__, count);
		goto fail;
	}

	if ((r=cmyth_rcv_string(conn, &err,
				reply, sizeof(reply)-1, count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_string() failed (%d)\n",
			  __FUNCTION__, r);
		goto fail;
	}
	count -= r;

	if ((r=cmyth_rcv_long(conn, &err, &port, count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_long() failed (%d)\n",
			  __FUNCTION__, r);
		goto fail;
	}

	if (port == -1)
		goto fail;

	rec->rec_id = id;
	rec->rec_server = ref_strdup(reply);
	rec->rec_port = port;
	rec->rec_connected = 1;

	if (cmyth_conn_connect_recorder(rec, conn->conn_buflen,
					conn->conn_tcp_rcvbuf) < 0)
		goto fail;

	pthread_mutex_unlock(&conn->conn_mutex);

	if (cmyth_recorder_add_chanlist(rec) < 0) {
		ref_release(rec);
		rec = NULL;
	}

	return rec;

    fail:
	if (rec)
		ref_release(rec);

	pthread_mutex_unlock(&conn->conn_mutex);

	return NULL;
}

cmyth_recorder_t
cmyth_conn_get_recorder(cmyth_conn_t conn, int num)
{
	cmyth_recorder_t rec = NULL;

	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n",
			  __FUNCTION__);
		return NULL;
	}

	pthread_mutex_lock(&conn->conn_mutex);

	if ((rec=cmyth_recorder_create()) == NULL)
		goto fail;

	rec->rec_id = num;
	rec->rec_server = NULL;
	rec->rec_port = 0;
	rec->rec_conn = ref_hold(conn);
	rec->rec_connected = 0;

	pthread_mutex_unlock(&conn->conn_mutex);

	if (cmyth_recorder_is_recording(rec) < 0) {
		ref_release(rec);
		rec = NULL;
	}

	if (cmyth_recorder_add_chanlist(rec) < 0) {
		ref_release(rec);
		rec = NULL;
	}

	return rec;

    fail:
	if (rec)
		ref_release(rec);

	pthread_mutex_unlock(&conn->conn_mutex);

	return NULL;
}

/*
 * cmyth_conn_get_free_recorder(cmyth_conn_t control, cmyth_recorder_t rec)
 *                             
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Obtain the next available free recorder the connection specified by
 * 'control'.  This fills out the recorder structure specified by 'rec'.
 *
 * Return Value:
 *
 * Success: 0 for not complete, 1 for complete
 *
 * Failure: -(errno)
 */
cmyth_recorder_t
cmyth_conn_get_free_recorder(cmyth_conn_t conn)
{
	int err, count;
	int r;
	long port, id;
	char msg[256];
	char reply[256];
	cmyth_recorder_t rec = NULL;

	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n",
			  __FUNCTION__);
		return NULL;
	}

	pthread_mutex_lock(&conn->conn_mutex);

	if ((rec=cmyth_recorder_create()) == NULL)
		goto fail;

	snprintf(msg, sizeof(msg), "GET_FREE_RECORDER");

	if ((err = cmyth_send_message(conn, msg)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message() failed (%d)\n",
			  __FUNCTION__, err);
		goto fail;
	}

	count = cmyth_rcv_length(conn);
	if (count < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_length() failed (%d)\n",
			  __FUNCTION__, count);
		goto fail;
	}
	if ((r=cmyth_rcv_long(conn, &err, &id, count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_long() failed (%d)\n",
			  __FUNCTION__, r);
		goto fail;
	}
	count -= r;
	if ((r=cmyth_rcv_string(conn, &err,
				reply, sizeof(reply)-1, count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_string() failed (%d)\n",
			  __FUNCTION__, r);
		goto fail;
	}
	count -= r;
	if ((r=cmyth_rcv_long(conn, &err, &port, count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_long() failed (%d)\n",
			  __FUNCTION__, r);
		goto fail;
	}

	if (port == -1)
		goto fail;

	rec->rec_id = id;
	rec->rec_server = ref_strdup(reply);
	rec->rec_port = port;
	rec->rec_connected = 1;

	if (cmyth_conn_connect_recorder(rec, conn->conn_buflen,
					conn->conn_tcp_rcvbuf) < 0)
		goto fail;

	pthread_mutex_unlock(&conn->conn_mutex);

	if (cmyth_recorder_add_chanlist(rec) < 0) {
		ref_release(rec);
		rec = NULL;
	}

	return rec;

    fail:
	if (rec)
		ref_release(rec);

	pthread_mutex_unlock(&conn->conn_mutex);

	return NULL;
}

int
cmyth_conn_get_freespace(cmyth_conn_t control,
			 long long *total, long long *used)
{
	int err, count, ret = 0;
	int r;
	char msg[256];
	char reply[256];
	int64_t lreply;

	if (control == NULL)
		return -EINVAL;

	if ((total == NULL) || (used == NULL))
		return -EINVAL;

	pthread_mutex_lock(&control->conn_mutex);

	if (control->conn_version >= 32)
		{ snprintf(msg, sizeof(msg), "QUERY_FREE_SPACE_SUMMARY"); }
	else if (control->conn_version >= 17)	
		{ snprintf(msg, sizeof(msg), "QUERY_FREE_SPACE"); }
	else
		{ snprintf(msg, sizeof(msg), "QUERY_FREESPACE"); }

	if ((err = cmyth_send_message(control, msg)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message() failed (%d)\n",
			  __FUNCTION__, err);
		ret = err;
		goto out;
	}

	if ((count=cmyth_rcv_length(control)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_length() failed (%d)\n",
			  __FUNCTION__, count);
		ret = count;
		goto out;
	}
	
	if (control->conn_version >= 17) {
		if ((r=cmyth_rcv_int64(control, &err, &lreply, count)) < 0) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: cmyth_rcv_int64() failed (%d)\n",
				  __FUNCTION__, err);
			ret = err;
			goto out;
		}
		count -= r;
		*total = lreply;
		if ((r=cmyth_rcv_int64(control, &err, &lreply, count)) < 0) {
			cmyth_dbg(CMYTH_DBG_ERROR,
				  "%s: cmyth_rcv_int64() failed (%d)\n",
				  __FUNCTION__, err);
			ret = err;
			goto out;
		}
		count -= r;
		*used = lreply;
	}
	else
		{
			if ((r=cmyth_rcv_string(control, &err, reply,
						sizeof(reply)-1, count)) < 0) {
				cmyth_dbg(CMYTH_DBG_ERROR,
					  "%s: cmyth_rcv_string() failed (%d)\n",
					  __FUNCTION__, err);
				ret = err;
				goto out;
			}
			count -= r;
			*total = atoi(reply);
			if ((r=cmyth_rcv_string(control, &err, reply,
						sizeof(reply)-1,
						count)) < 0) {
				cmyth_dbg(CMYTH_DBG_ERROR,
					  "%s: cmyth_rcv_string() failed (%d)\n",
					  __FUNCTION__, err);
				ret = err;
				goto out;
			}
			count -= r;
			*used = atoi(reply);

			*used *= 1024;
			*total *= 1024;
		}

	if (count != 0) {
		ret = -1;
		cmyth_dbg(CMYTH_DBG_ERROR, "%s(): %d extra bytes\n",
			  __FUNCTION__, count);
	}

    out:
	pthread_mutex_unlock(&control->conn_mutex);

	return ret;
}

int
cmyth_conn_hung(cmyth_conn_t control)
{
	if (control == NULL)
		return -EINVAL;

	return control->conn_hang;
}

int
cmyth_conn_get_protocol_version(cmyth_conn_t conn)
{
	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n",
			__FUNCTION__);
		return -1;
	}

	return conn->conn_version;
}


int
cmyth_conn_get_free_recorder_count(cmyth_conn_t conn)
{
	char msg[256];
	int count, err;
	long c, r;
	int ret;

	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n",
			  __FUNCTION__);
		return -1;
	}

	pthread_mutex_lock(&conn->conn_mutex);

	snprintf(msg, sizeof(msg), "GET_FREE_RECORDER_COUNT");
	if ((err = cmyth_send_message(conn, msg)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message() failed (%d)\n",
			  __FUNCTION__, err);
		ret = err;
		goto err;
	}

	if ((count=cmyth_rcv_length(conn)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_length() failed (%d)\n",
			  __FUNCTION__, count);
		ret = count;
		goto err;
	}
	if ((r=cmyth_rcv_long(conn, &err, &c, count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_long() failed (%d)\n",
			  __FUNCTION__, r);
		ret = err;
		goto err;
	}

	ret = c;

    err:
	pthread_mutex_unlock(&conn->conn_mutex);

	return ret;
}

static char *
cmyth_conn_get_setting_unlocked(cmyth_conn_t conn, const char* hostname, const char* setting)
{
	char msg[256];
	int count, err;
	char* result = NULL;

	if(conn->conn_version < 17) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: protocol version doesn't support QUERY_SETTING\n",
			  __FUNCTION__);
		return NULL;
	}

	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n",
			  __FUNCTION__);
		return NULL;
	}

	snprintf(msg, sizeof(msg), "QUERY_SETTING %s %s", hostname, setting);
	if ((err = cmyth_send_message(conn, msg)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message() failed (%d)\n",
			  __FUNCTION__, err);
		goto err;
	}

	if ((count=cmyth_rcv_length(conn)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_length() failed (%d)\n",
			  __FUNCTION__, count);
		goto err;
	}

	result = ref_alloc(count+1);
	count -= cmyth_rcv_string(conn, &err,
				    result, count, count);
	if (err < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_rcv_string() failed (%d)\n",
			  __FUNCTION__, err);
		goto err;
	}

	while(count > 0 && !err) {
		char buffer[100];
		count -= cmyth_rcv_string(conn, &err, buffer, sizeof(buffer)-1, count);
		buffer[sizeof(buffer)-1] = 0;
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: odd left over data %s\n", __FUNCTION__, buffer);
	}

	return result;
err:
	if(result)
		ref_release(result);

	return NULL;
}

char *
cmyth_conn_get_setting(cmyth_conn_t conn, const char* hostname, const char* setting)
{
	char* result = NULL;

	pthread_mutex_lock(&conn->conn_mutex);
	result = cmyth_conn_get_setting_unlocked(conn, hostname, setting);
	pthread_mutex_unlock(&conn->conn_mutex);

	return result;
}

static int
okay_command(cmyth_conn_t conn, char *msg, unsigned int min_version)
{
	int err;
	int rc = 0;

	if (!conn) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n",
			  __FUNCTION__);
		return -1;
	}

	if (conn->conn_version < min_version) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: protocol version doesn't support %s\n",
			  __FUNCTION__, msg);
		return -1;
	}

	pthread_mutex_lock(&conn->conn_mutex);

	if ((err = cmyth_send_message(conn, msg)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			  "%s: cmyth_send_message() failed (%d)\n",
			  __FUNCTION__, err);
		rc = -1;
		goto err;
	}

	if (cmyth_rcv_okay(conn) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: cmyth_rcv_okay() failed\n",
			  __FUNCTION__);
		rc = -1;
		goto err;
	}

err:
	pthread_mutex_unlock(&conn->conn_mutex);

	return rc;
}

int
cmyth_conn_allow_shutdown(cmyth_conn_t conn)
{
	return okay_command(conn, "ALLOW_SHUTDOWN", 18);
}

int
cmyth_conn_block_shutdown(cmyth_conn_t conn)
{
	return okay_command(conn, "BLOCK_SHUTDOWN", 18);
}
