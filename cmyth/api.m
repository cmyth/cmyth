/*
 *  Copyright (C) 2009-2010, Jon Gettler
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

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <cmyth/cmyth.h>
#include <refmem/refmem.h>

#import "api.h"

@implementation cmythProgram

-(cmythProgram*)program:(cmyth_proginfo_t)program
{
	self = [super init];

	prog = program;

	return self;
}

-(cmyth_proginfo_t)proginfo
{
	return prog;
}

#define proginfo_method(type)						\
-(NSString*)type							\
{									\
	char *type;							\
	NSString *result = nil;						\
									\
	type = cmyth_proginfo_##type(prog);				\
									\
	if (type != NULL) {						\
		result = [[NSString alloc] initWithUTF8String:type];	\
		ref_release(type);					\
	}								\
									\
	return result;							\
}

proginfo_method(title)
proginfo_method(subtitle)
proginfo_method(description)
proginfo_method(category)
proginfo_method(pathname)

-(NSString*)date
{
	cmyth_timestamp_t ts;
	time_t t;

	ts = cmyth_proginfo_rec_start(prog);
	t = cmyth_timestamp_to_unixtime(ts);

	NSDate *date = [NSDate dateWithTimeIntervalSince1970:t];

	ref_release(ts);

	return [date description];
}

-(int)seconds
{
	cmyth_timestamp_t ts, te;
	time_t t;

	ts = cmyth_proginfo_rec_start(prog);
	te = cmyth_proginfo_rec_end(prog);

	t = cmyth_timestamp_to_unixtime(te) - cmyth_timestamp_to_unixtime(ts);

	ref_release(ts);
	ref_release(te);

	return t;
}

-(void) dealloc
{
	ref_release(prog);

	[super dealloc];
}

@end

@implementation cmythFile

@synthesize lock;

typedef struct {
	char *command;
	char *file;
	long long start;
	long long end;
} url_t;

static url_t*
read_header(int fd)
{
	char buf[512];
	char request[512];
	char line[512];
	int l = 0;
	url_t *u;
	char *p, *b, *o;
	static int seen = 0;

	read(fd, buf, sizeof(buf));

	u = (url_t*)malloc(sizeof(url_t));
	memset(u, 0, sizeof(*u));

	memset(request, 0, sizeof(request));
	memset(line, 0, sizeof(line));

	b = strstr(buf, "\r\n");
	*b = '\0';
	strcpy(request, buf);
	*b = '\r';
	b += 2;

	u->command = strdup(request);
	if ((p=strchr(u->command, ' ')) != NULL) {
		*(p++) = '\0';
		u->file = p;
		if ((p=strchr(p, ' ')) != NULL) {
			*(p++) = '\0';
		}
	}

	while (1) {
		char *field, *value;

		o = b;
		b = strstr(o, "\r\n");
		*b = '\0';
		strcpy(line, o);
		*b = '\r';
		b += 2;
		if (*b == '\r') {
			break;
		}

		if (strcmp(line, "\r\n") == 0) {
			break;
		}

		field = line;
		if ((value=strchr(field, ':')) != NULL) {
			*(value++) = '\0';
		}

		if (strcasecmp(field, "range") == 0) {
			char *start, *end;

			start = strchr(value, '=');
			start++;
			end = strchr(start, '-');
			*(end++) = '\0';

			u->start = strtoll(start, NULL, 0);
			u->end = strtoll(end, NULL, 0);
		}

		l++;
	}

	seen++;

	return u;
}

static void
handler(int sig)
{
}

static int
my_write(int fd, char *buf, int len)
{
	int tot = 0;
	int n, err;

	while (tot < len) {
		n = write(fd, buf+tot, len-tot);
		err = errno;
		if (n < 0) {
			break;
		}

		tot += n;
	}

	return tot;
}

static int
send_header(int fd, url_t *u, long long length)
{
	long long size = u->end - u->start + 1;
	char buf[512];

	memset(buf, 0, sizeof(buf));

	sprintf(buf, "HTTP/1.1 206 Partial Content\r\n");
	sprintf(buf+strlen(buf), "Server: mvpmc\r\n");
	sprintf(buf+strlen(buf), "Accept-Ranges: bytes\r\n");
	sprintf(buf+strlen(buf), "Content-Length: %lld\r\n", size);
	sprintf(buf+strlen(buf), "Content-Range: bytes %lld-%lld/%lld\r\n",
		u->start, u->end, length);
	sprintf(buf+strlen(buf), "Connection: close\r\n");
	sprintf(buf+strlen(buf), "\r\n");

	if (my_write(fd, buf, strlen(buf)) != strlen(buf)) {
		return -1;
	}

	return 0;
}

static long long
send_data(int fd, url_t *u, cmyth_file_t file)
{
	unsigned char *buf;
	unsigned long long pos;
	long long wrote = 0;

#define BSIZE (8*1024)
	if ((buf=(unsigned char*)malloc(BSIZE)) == NULL) {
		return;
	}

	pos = cmyth_file_seek(file, u->start, SEEK_SET);

	while (pos <= (u->end)) {
		int size, tot, len;

		size = ((u->end - pos) >= BSIZE) ? BSIZE : (u->end - pos + 1);

		len = cmyth_file_request_block(file, size);

		tot = 0;
		while (tot < len) {
			int n;
			n = cmyth_file_get_block(file, buf+tot, len-tot);
			if (n <= 0) {
				goto err;
			}
			tot += n;
		}

		if (my_write(fd, buf, len) != len) {
			break;
		}

		wrote += len;

		pos += len;
	}

err:
	free(buf);

	return wrote;
}

-(void)server
{
	int fd;
	int attempts = 0;
	long long wrote = 0;

	while (1) {
		url_t *u;

		if (listen(sockfd, 5) != 0) {
			return;
		}

		if ((fd=accept(sockfd, NULL, NULL)) < 0) {
			return;
		}

		int set = 1;
		setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));

		attempts++;

		u = read_header(fd);

		if (strcasecmp(u->command, "GET") == 0) {
			if (send_header(fd, u, length) == 0) {
				wrote += send_data(fd, u, file);
			}
		}

		close(fd);
	}
}

static int
create_socket(int *f, int *p)
{
	int fd, port, rc;
	int attempts = 0;
	struct sockaddr_in sa;

	if ((fd=socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		return -1;
	}

	int set = 1;
	setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));

	do {
		port = (random() % 32768) + 5001;

		memset(&sa, 0, sizeof(sa));

		sa.sin_family = AF_INET;
		sa.sin_port = htons(port);
		sa.sin_addr.s_addr = INADDR_ANY;

		rc = bind(fd, &sa, sizeof(sa));
	} while ((rc != 0) && (attempts++ < 100));

	if (rc != 0) {
		close(fd);
		return -1;
	}

	*f = fd;
	*p = port;

	return 0;
}

-(cmythFile*)openWith:(cmythProgram*)program
{
	int tcp_control = 4096;
	int tcp_program = 128*1024;
	int port;
	char *host;
	cmyth_proginfo_t prog;

	if (create_socket(&sockfd, &portno) != 0) {
		return nil;
	}

	prog = [program proginfo];

	if ((host=cmyth_proginfo_host(prog)) == NULL) {
		goto err;
	}

	if ((port=cmyth_proginfo_port(prog)) < 0) {
		goto err;
	}

	if ((conn=cmyth_conn_connect_ctrl(host, port, 16*1024,
					  tcp_control)) == NULL) {
		goto err;
	}

#define MAX_BSIZE	(256*1024*3)
	if ((file=cmyth_conn_connect_file(prog, conn, MAX_BSIZE,
					  tcp_program)) == NULL) {
		goto err;
	}

	length = cmyth_proginfo_length(prog);

	self = [super init];

	[NSThread detachNewThreadSelector:@selector(server)
		  toTarget:self withObject:nil];

	return self;

err:
	close(sockfd);

	return nil;
}

static int send_password(int fd)
{
	char buf[128];
	char *passwd = "admin\n";

	memset(buf, 0, sizeof(buf));

	struct timeval tv;
	fd_set fds;

	tv.tv_sec = 5;
	tv.tv_usec = 0;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	if (select(fd + 1, &fds, NULL, NULL, &tv) < 0) {
		return -1;
	}

	errno = 0;
	if (read(fd, buf, sizeof(buf)) <= 0) {
		return -1;
	}

	if (strncmp(buf, "Password:", 9) != 0) {
		return -1;
	}

	my_write(fd, passwd, strlen(passwd));

	memset(buf, 0, sizeof(buf));

	read(fd, buf, sizeof(buf));

	if (strstr(buf, "> ") == NULL) {
		return -1;
	}

	return 0;
}

static void sighandler(int sig)
{
}

static int issue_command(int fd, char *buf)
{
	struct timeval tv;
	fd_set fds;
	char input[512];

	if (my_write(fd, buf, strlen(buf)) != strlen(buf)) {
		return -1;
	}

	tv.tv_sec = 0;
	tv.tv_usec = 100;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	if (select(fd + 1, &fds, NULL, NULL, &tv) < 0) {
		return -1;
	}

	read(fd, input, sizeof(input));

	return 0;
}

static int command_result(int fd, char *buf, char *result, int max)
{
	struct timeval tv;
	fd_set fds;
	int len;
	char *p;

	if (my_write(fd, buf, strlen(buf)) != strlen(buf)) {
		return -1;
	}

	len = 0;
	memset(result, 0, max);
	max--;
	while (len < max) {
		char *p;
		int n;

		tv.tv_sec = 1;
		tv.tv_usec = 0;

		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		if (select(fd + 1, &fds, NULL, NULL, &tv) <= 0) {
			break;
		}

		n = read(fd, result+len, max-len);

		if (n <= 0) {
			break;
		}

		len += n;
	}

	return len;
}

static int send_commands(int fd, char *src, char *dest, char *file)
{
	char *cmd_del = "del %s\n";
	char *cmd_new = "new %s broadcast enabled\n";
	char *cmd_input = "setup %s input %s/%s\n";
	char *cmd_output = "setup %s output #transcode{width=480,canvas-height=320,vcodec=mp4v,vb=768,acodec=mp4a,ab=192,channels=2}:standard{access=file,mux=mp4,dst=%s/%s.mp4}\n";
	char *cmd_play = "control %s play\n";
	char buf[512], id[128];

	snprintf(id, sizeof(id), "mvpmc.iphone.%s", file);

	snprintf(buf, sizeof(buf), cmd_del, id);
	if (issue_command(fd, buf) != 0) {
		return -1;
	}

	snprintf(buf, sizeof(buf), cmd_new, id);
	if (issue_command(fd, buf) != 0) {
		return -1;
	}

	snprintf(buf, sizeof(buf), cmd_input, id, src, file);
	if (issue_command(fd, buf) != 0) {
		return -1;
	}

	snprintf(buf, sizeof(buf), cmd_output, id, dest, file);
	if (issue_command(fd, buf) != 0) {
		return -1;
	}

	snprintf(buf, sizeof(buf), cmd_play, id);
	if (issue_command(fd, buf) != 0) {
		return -1;
	}

	return 0;
}

-(void)transcoder
{
	char *h = [vlc UTF8String];
	int fd, ret;
	struct sockaddr_in sa;
	struct hostent* server;

	server = gethostbyname(h);

	if ((fd=socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		self->state = CMYTH_TRANSCODE_CONNECT_FAILED;
		return;
	}

	sa.sin_family = AF_INET;
	sa.sin_port = htons(4212);
	memcpy((char*)&sa.sin_addr,(char*)server->h_addr,server->h_length);

	int set = 1;
	setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));

	void (*old_sighandler)(int);
	int old_alarm;

	NSLog(@"VLC connect to %@", vlc);
	old_sighandler = signal(SIGALRM, sighandler);
	old_alarm = alarm(5);
	ret = connect(fd, (struct sockaddr *)&sa, sizeof(sa));
	signal(SIGALRM, old_sighandler);
	alarm(old_alarm);

	if (ret < 0) {
		NSLog(@"VLC connect failed");
		close(fd);
		self->state = CMYTH_TRANSCODE_CONNECT_FAILED;
		return;
	}

	setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));

	if (send_password(fd) != 0) {
		NSLog(@"VLC login failed");
		close(fd);
		self->state = CMYTH_TRANSCODE_CONNECT_FAILED;
		return;
	}

	NSLog(@"VLC password accepted");

	self->state = CMYTH_TRANSCODE_STARTING;

	NSString *file = [program pathname];
	ret = send_commands(fd, [srcPath UTF8String],
			    [dstPath UTF8String], [file UTF8String]);

	if (ret == 0) {
		state = CMYTH_TRANSCODE_IN_PROGRESS;
	} else {
		state = CMYTH_TRANSCODE_ERROR;
		close(fd);
		return;
	}

	char *fn = [file UTF8String];
	char *pos = "position : ";
	while (!done) {
		char id[256], cmd[512], line[256], output[4096];
		int n;

		sleep(1);
		snprintf(id, sizeof(id), "mvpmc.iphone.%s", fn);
		snprintf(cmd, sizeof(cmd), "show %s\n", id);

		if ((n=command_result(fd, cmd, output, sizeof(output))) > 0) {
			char *p = strstr(output, pos);
			if (p != NULL) {
				char *R = strchr(p, '\r');
				char *N = strchr(p, '\n');

				if (R) {
					*R = '\0';
				}
				if (N) {
					*N = '\0';
				}

				p += strlen(pos);
				progress = strtof(p, NULL);
			} else {
				if (strchr(p, "> ") != NULL) {
					progress = 1;
					break;
				}
			}
		} else {
			NSLog(@"VLC read failed with n %d",n);
		}
	}

	if (done == 1) {
		char id[256], cmd[512];

		snprintf(id, sizeof(id), "mvpmc.iphone.%s", fn);
		snprintf(cmd, sizeof(cmd), "del %s\n", id);

		NSLog(@"VLC %s",cmd);

		issue_command(fd, cmd);
	}

	close(fd);

	[lock lock];
	done = 2;
	[lock unlock];
}

-(cmythFile*)transcodeWith:(cmythProgram*)program
		  mythPath:(NSString*)myth
		   vlcHost:(NSString*)host
		   vlcPath:(NSString*)path
{
	cmythFile *f = nil;

	if ((program == nil) || (myth == nil) ||
	    (host == nil) || (path == nil)) {
		return nil;
	}

	self = [super init];

	self->state = CMYTH_TRANSCODE_UNKNOWN;
	self->program = program;
	self->srcPath = myth;
	self->dstPath = path;
	self->vlc = host;

	[NSThread detachNewThreadSelector:@selector(transcoder)
		  toTarget:self withObject:nil];

	return self;
}

-(void)transcodeStop
{
	[lock lock];
	if (done == 0) {
		done = 1;
	}
	[lock unlock];

	while (done == 1) {
		usleep(100);
	}
}

-(float)transcodeProgress
{
	float ret;

	switch (state) {
	case CMYTH_TRANSCODE_COMPLETE:
		ret = 1.0;
		break;
	case CMYTH_TRANSCODE_IN_PROGRESS:
		ret = progress;
		break;
	default:
		ret = 0.0;
		break;
	}

	return ret;
}

-(int)portNumber
{
	return portno;
}

-(cmythTranscodeState)transcodeState
{
	return state;
}

@end

@implementation cmythProgramList

-(cmythProgramList*)control:(cmyth_conn_t)control
{
	cmyth_proglist_t list;
	int i, count;

	if ((list=cmyth_proglist_get_all_recorded(control)) == NULL) {
		return nil;
	}

	self = [super init];

	count = cmyth_proglist_get_count(list);

	array = [[NSMutableArray alloc] init];

	for (i=0; i<count; i++) {
		cmyth_proginfo_t prog;
		cmythProgram *program;

		prog = cmyth_proglist_get_item(list, i);
		program = [[cmythProgram alloc] program:prog];

		[array addObject: program];
	}

	return self;
}

-(cmythProgram*)progitem:(int)n
{
	return [array objectAtIndex:n];
}

-(int) count
{
	return [array count];
}

-(void) dealloc
{
	[array release];
	[super dealloc];
}

@end

@implementation cmyth

-(cmyth*) server:(NSString*) server
	    port: (unsigned short) port
{
	cmyth_conn_t c, e;
	char *host = [server UTF8String];
	int len = 16*1024;
	int tcp = 4096;

	if (port == 0) {
		port = 6543;
	}

	if (host == NULL) {
		return nil;
	}

	if ((c=cmyth_conn_connect_ctrl(host, port, len, tcp)) == NULL) {
		return nil;
	}
	if ((e=cmyth_conn_connect_event(host, port, len, tcp)) == NULL) {
		return nil;
	}

	self = [super init];

	if (self) {
		control = c;
		event = e;
	} else {
		control = NULL;
		event = NULL;
	}

	return self;
}

-(int) protocol_version
{
	return cmyth_conn_get_protocol_version(control);
}

-(cmythProgramList*)programList
{
	cmythProgramList *list;

	list = [[cmythProgramList alloc] control:control];

	return list;
}

-(int)getEvent:(cmyth_event_t*)event
{
	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = 0;

	if (cmyth_event_select(self->event, &tv) > 0) {
		*event = cmyth_event_get(self->event, NULL, 0);
		return 0;
	}

	return -1;
}

-(void) dealloc
{
	ref_release(control);
	ref_release(event);

	[super dealloc];
}

@end
