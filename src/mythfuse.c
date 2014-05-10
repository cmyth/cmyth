/*
 *  Copyright (C) 2008-2014, Jon Gettler
 *  http://www.mvpmc.org/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <libgen.h>
#include <getopt.h>
#include <pthread.h>

#include <cmyth/cmyth.h>
#include <refmem/refmem.h>

#define MAX_CONN	32
#define MAX_FILES	32
#define MAX_BSIZE	(128*1024)
#define MIN_BSIZE	(1024*2)

struct prog_map {
	cmyth_proginfo_t prog;
	unsigned int suffix;
};

struct myth_conn {
	char *host;
	cmyth_conn_t control;
	cmyth_conn_t event;
	cmyth_proglist_t list;
	pthread_t thread;
	int used;
	struct prog_map *progs;
	int nprogs;
};

struct path_info {
	char *host;
	char *dir;
	char *file;
};

struct file_info {
	cmyth_file_t file;
	off_t offset;
	struct path_info *info;
	char *buf;
	size_t n;
	off_t start;
	int used;
};

struct dir_cb {
	char *name;
	int (*readdir)(struct path_info*, void*, fuse_fill_dir_t,
		       off_t, struct fuse_file_info*);
	int (*getattr)(struct path_info*, struct stat*);
	int (*open)(int, struct path_info*, struct fuse_file_info *fi);
};

static struct myth_conn conn[MAX_CONN];

static struct file_info files[MAX_FILES];

static FILE *F = NULL;

static int tcp_control = 4096;
static int tcp_program = 128*1024;
static int port = 6543;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static int rd_files(struct path_info*, void*, fuse_fill_dir_t, off_t,
		    struct fuse_file_info*);
static int ga_files(struct path_info*, struct stat*);
static int o_files(int, struct path_info*, struct fuse_file_info *fi);
static int rd_all(struct path_info*, void*, fuse_fill_dir_t, off_t,
		  struct fuse_file_info*);
static int ga_all(struct path_info*, struct stat*);

static struct dir_cb dircb[] = {
	{ "files", rd_files, ga_files, o_files },
	{ "all", rd_all, ga_all, NULL },
	{ NULL, NULL, NULL, NULL },
};

static struct option opts[] = {
	{ "debug", required_argument, 0, 'd' },
	{ "help", required_argument, 0, 'h' },
	{ 0, 0, 0, 0 }
};

#define debug(fmt...) 					\
	{						\
		if (F != NULL) {			\
			fprintf(F, fmt);		\
			fflush(F);			\
		}					\
	}

static char *README =
	"This is the mythfuse filesystem.\n\n"
	"Directories will be created dynamically in the root directory of\n"
	"the filesystem as they are accessed.  Their names will be the IP\n"
	"address or hostname of the MythTV backend being accessed.\n";
static char *README_PATH = "/README";

static time_t readme_time = 0;
static time_t readme_atime = 0;

static void
free_info(struct path_info *info)
{
	free(info->host);
	free(info->dir);
	free(info->file);
}

static void
parse_progs(struct myth_conn *c)
{
	int i, j, count;
	struct prog_map *progs;

	count = cmyth_proglist_get_count(c->list);

	progs = ref_alloc(sizeof(*progs)*count);

	for (i=0; i<count; i++) {
		cmyth_proginfo_t prog;
		char *title, *subtitle;
		prog = cmyth_proglist_get_item(c->list, i);
		title = cmyth_proginfo_title(prog);
		subtitle = cmyth_proginfo_subtitle(prog);
		progs[i].prog = ref_hold(prog);
		progs[i].suffix = 0;
		for (j=0; j<i; j++) {
			char *t, *s;
			t = cmyth_proginfo_title(progs[j].prog);
			s = cmyth_proginfo_subtitle(progs[j].prog);
			if ((strcmp(title, t) == 0) &&
			    (strcmp(subtitle, s) == 0)) {
				progs[i].suffix++;
			}
			ref_release(t);
			ref_release(s);
		}
		ref_release(title);
		ref_release(subtitle);
		ref_release(prog);
	}

	for (i=0; i<c->nprogs; i++) {
		ref_release(c->progs[i].prog);
	}
	ref_release(c->progs);
	c->progs = progs;
}

static void*
event_loop(void *arg)
{
	intptr_t i = (intptr_t)arg;
	char buf[128];
	cmyth_event_t next;
	int done = 0;
	cmyth_conn_t event;
	cmyth_conn_t control;
	cmyth_proglist_t list;

	debug("%s(): event loop started\n", __FUNCTION__);

	pthread_mutex_lock(&mutex);

	pthread_detach(pthread_self());

	if (!conn[i].used) {
		pthread_mutex_unlock(&mutex);
		return NULL;
	}

	event = conn[i].event;
	control = conn[i].control;
	list = conn[i].list;

	pthread_mutex_unlock(&mutex);

	while (!done) {
		next = cmyth_event_get(event, buf, 128);

		pthread_mutex_lock(&mutex);

		switch (next) {
		case CMYTH_EVENT_CLOSE:
			ref_release(control);
			ref_release(list);
			ref_release(event);
			done = 1;
			break;
		case CMYTH_EVENT_RECORDING_LIST_CHANGE:
			ref_release(list);
			list = cmyth_proglist_get_all_recorded(control);
			conn[i].list = list;
			parse_progs(conn+i);
			break;
		default:
			break;
		}

		pthread_mutex_unlock(&mutex);
	}

	return NULL;
}

static int
lookup_server(char *host)
{
	intptr_t i, j = -1;
	cmyth_conn_t control, event;

	debug("%s(): host '%s'\n", __FUNCTION__, host);

	for (i=0; i<MAX_CONN; i++) {
		if (conn[i].host && (strcmp(conn[i].host, host) == 0)) {
			break;
		}
		if ((j < 0) && (!conn[i].used)) {
			j = i;
		}
	}

	if (i == MAX_CONN) {
		if (j < 0) {
			debug("%s(): error at %d\n", __FUNCTION__, __LINE__);
			return -1;
		}
		conn[j].used = 1;
	}

	if (i == MAX_CONN) {
		if ((control=cmyth_conn_connect_ctrl(host, port, 16*1024,
						     tcp_control)) == NULL) {
			debug("%s(): error at %d\n", __FUNCTION__, __LINE__);
			conn[j].used = 0;
			return -1;
		}
		if ((event=cmyth_conn_connect_event(host, port, 16*1024,
						    tcp_control)) == NULL) {
			debug("%s(): error at %d\n", __FUNCTION__, __LINE__);
			conn[j].used = 0;
			return -1;
		}

		conn[j].host = strdup(host);
		conn[j].control = control;
		conn[j].event = event;
		conn[j].list = NULL;

		pthread_create(&conn[j].thread, NULL, event_loop, (void*)j);

		i = j;
	}

	return i;
}

static int
lookup_path(const char *path, struct path_info *info)
{
	char *f, *p;
	char *tmp = strdup(path);
	int ret = 0;
	char *parts[8];
	int i = 0, j;

	memset(info, 0, sizeof(*info));

	if (strcmp(path, "/") == 0) {
		goto out;
	}

	memset(parts, 0, sizeof(parts));

	p = tmp;
	do {
		if ((f=strchr(p+1, '/')) != NULL) {
			*f = '\0';
			parts[i++] = strdup(p+1);
			p = f;
		}
	} while ((f != NULL) && (i < 8));

	debug("%s(): found %d parts\n", __FUNCTION__, i);

	if (i == 8) {
		ret = -ENOENT;
		goto out;
	}

	parts[i] = strdup(p+1);

	if ((i >= 2) && 
	    ((strcmp(parts[1], "files") != 0) &&
	     (strcmp(parts[1], "all") != 0))) {
		for (j=0; j<=i; j++) {
			free(parts[j]);
		}
		ret = -ENOENT;
		goto out;
	}

	info->host = parts[0];
	info->dir = parts[1];
	info->file = parts[2];

	for (j=3; j<i; j++) {
		free(parts[j]);
	}

	debug("%s(): host '%s' file '%s'\n", __FUNCTION__,
	      info->host, info->file);

out:
	if (tmp)
		free(tmp);

	debug("%s(): ret %d\n", __FUNCTION__, ret);

	return ret;
}

static void *myth_init(struct fuse_conn_info *conn)
{
	debug("%s(): start\n", __FUNCTION__);

	return NULL;
}

static void myth_destroy(void *arg)
{
	debug("%s(): stop\n", __FUNCTION__);

#if 0
	int i;

	for (i=0; i<MAX_CONN; i++) {
		if (conn[i].list) {
			ref_release(conn[i].list);
		}
		if (conn[i].control) {
			ref_release(conn[i].control);
		}
	}
#endif
}

static int
do_open(cmyth_proginfo_t prog, struct fuse_file_info *fi, int i)
{
	cmyth_conn_t c = NULL;
	cmyth_file_t f = NULL;
	char *host;

	if ((host=cmyth_proginfo_host(prog)) == NULL) {
		return -1;
	}

	if ((c=cmyth_conn_connect_ctrl(host, port, 16*1024,
				       tcp_control)) == NULL) {
		return -1;
	}

	if ((f=cmyth_conn_connect_file(prog, c, MAX_BSIZE,
				       tcp_program)) == NULL) {
		return -1;
	}

	ref_release(host);
	ref_release(c);

	files[i].file = f;
	files[i].buf = malloc(MAX_BSIZE);
	files[i].start = 0;
	files[i].n = 0;

	fi->fh = i;

	return 0;
}

static int o_files(int f, struct path_info *info, struct fuse_file_info *fi)
{
	int i;
	cmyth_conn_t control;
	cmyth_proglist_t list;
	int count;
	int ret = -ENOENT;

	pthread_mutex_lock(&mutex);

	if ((i=lookup_server(info->host)) < 0) {
		pthread_mutex_unlock(&mutex);
		return -ENOENT;
	}

	control = ref_hold(conn[i].control);

	if (conn[i].list == NULL) {
		list = cmyth_proglist_get_all_recorded(control);
		conn[i].list = list;
		parse_progs(conn+i);
	} else {
		list = conn[i].list;
	}

	list = ref_hold(list);

	pthread_mutex_unlock(&mutex);

	count = cmyth_proglist_get_count(list);

	for (i=0; i<count; i++) {
		cmyth_proginfo_t prog;
		char *pn;

		prog = cmyth_proglist_get_item(list, i);
		pn = cmyth_proginfo_pathname(prog);

		if (strcmp(pn+1, info->file) == 0) {
			if (do_open(prog, fi, f) < 0) {
				ref_release(pn);
				ref_release(prog);
				goto out;
			}
			ref_release(pn);
			ref_release(prog);
			ret = 0;
			goto out;
		}

		ref_release(pn);
		ref_release(prog);
	}

out:
	ref_release(control);
	ref_release(list);

	return ret;
}

static int readme_open(const char *path, struct fuse_file_info *fi)
{
	time(&readme_atime);

	return 0;
}

static int myth_open(const char *path, struct fuse_file_info *fi)
{
	int i, f;
	struct path_info info;

	debug("%s(): path '%s'\n", __FUNCTION__, path);

	if (strcmp(path, README_PATH) == 0) {
		return readme_open(path, fi);
	}

	memset(&info, 0, sizeof(info));
	if (lookup_path(path, &info) < 0) {
		return -ENOENT;
	}

	if (info.file == NULL) {
		return -ENOENT;
	}

	pthread_mutex_lock(&mutex);

	for (f=0; f<MAX_FILES; f++) {
		if (!files[f].used) {
			break;
		}
	}
	if (f == MAX_FILES) {
		pthread_mutex_unlock(&mutex);
		return -ENFILE;
	}

	files[f].used = 1;

	pthread_mutex_unlock(&mutex);

	fi->fh = -1;

	i = 0;
	while (dircb[i].name) {
		if (strcmp(info.dir, dircb[i].name) == 0) {
			if (dircb[i].open == NULL) {
				return -ENOENT;
			}
			dircb[i].open(f, &info, fi);
			return 0;
		}
		i++;
	}

	return -ENOENT;
}

static int myth_release(const char *path, struct fuse_file_info *fi)
{
	cmyth_file_t f;
	int i = fi->fh;

	debug("%s(): path '%s'\n", __FUNCTION__, path);

	if ((int)fi->fh != -1) {
		f = files[i].file;
		ref_release(f);
		if (files[i].buf) {
			free(files[i].buf);
		}
		memset(files+i, 0, sizeof(struct file_info));
		fi->fh = -1;
	}

	return 0;
}

static int
rd_files(struct path_info *info, void *buf, fuse_fill_dir_t filler,
	 off_t offset, struct fuse_file_info *fi)
{
	int i;
	cmyth_conn_t control;
	cmyth_proglist_t list;
	int count;

	pthread_mutex_lock(&mutex);

	if ((i=lookup_server(info->host)) < 0) {
		pthread_mutex_unlock(&mutex);
		return 0;
	}

	control = ref_hold(conn[i].control);

	if (conn[i].list == NULL) {
		list = cmyth_proglist_get_all_recorded(control);
		conn[i].list = list;
		parse_progs(conn+i);
	} else {
		list = conn[i].list;
	}

	list = ref_hold(list);

	pthread_mutex_unlock(&mutex);

	count = cmyth_proglist_get_count(list);

	for (i=0; i<count; i++) {
		cmyth_proginfo_t prog;
		long long len;
		char *fn, *pn;

		prog = cmyth_proglist_get_item(list, i);
		pn = cmyth_proginfo_pathname(prog);
		len = cmyth_proginfo_length(prog);

		fn = pn+1;

		debug("%s(): file '%s' len %lld\n", __FUNCTION__, fn, len);
		filler(buf, fn, NULL, 0);

		ref_release(prog);
		ref_release(pn);
	}

	ref_release(control);
	ref_release(list);

	return 0;
}

static int
rd_all(struct path_info *info, void *buf, fuse_fill_dir_t filler,
       off_t offset, struct fuse_file_info *fi)
{
	int i;
	cmyth_conn_t control;
	cmyth_proglist_t list;
	int count;
	struct myth_conn *mc;

	pthread_mutex_lock(&mutex);

	if ((i=lookup_server(info->host)) < 0) {
		pthread_mutex_unlock(&mutex);
		return 0;
	}

	control = ref_hold(conn[i].control);

	if (conn[i].list == NULL) {
		list = cmyth_proglist_get_all_recorded(control);
		conn[i].list = list;
		parse_progs(conn+i);
	} else {
		list = conn[i].list;
	}

	mc = conn+i;

	list = ref_hold(list);

	pthread_mutex_unlock(&mutex);

	count = cmyth_proglist_get_count(list);

	char *name[count];

	for (i=0; i<count; i++) {
		cmyth_proginfo_t prog;
		long long len;
		char *fn, *pn, *t, *s;

		prog = cmyth_proglist_get_item(list, i);
		pn = cmyth_proginfo_pathname(prog);
		t = cmyth_proginfo_title(prog);
		s = cmyth_proginfo_subtitle(prog);
		len = cmyth_proginfo_length(prog);

		if (mc->progs[i].suffix == 0) {
			name[i] = ref_sprintf("%s - %s.nuv", t, s);
		} else {
			name[i] = ref_sprintf("%s - %s (%d).nuv", t, s,
					      mc->progs[i].suffix);
		}

		fn = pn+1;

		debug("%s(): file '%s' len %lld\n", __FUNCTION__, fn, len);
		filler(buf, name[i], NULL, 0);

		ref_release(prog);
		ref_release(pn);
		ref_release(t);
		ref_release(s);
	}

	for (i=0; i<count; i++) {
		ref_release(name[i]);
	}

	ref_release(control);
	ref_release(list);

	return 0;
}

static int myth_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			off_t offset, struct fuse_file_info *fi)
{
	struct path_info info;
	int i;

	debug("%s(): path '%s'\n", __FUNCTION__, path);

	memset(&info, 0, sizeof(info));
	if (lookup_path(path, &info) < 0) {
		return -ENOENT;
	}

	if (info.host == NULL) {
		for (i=0; i<MAX_CONN; i++) {
			if (conn[i].host) {
				filler(buf, conn[i].host, NULL, 0);
			}
		}

		goto finish;
	}

	if (info.dir == NULL) {
		i = 0;
		while (dircb[i].name) {
			filler(buf, dircb[i].name, NULL, 0);
			i++;
		}
		goto finish;
	}

	i = 0;
	while (dircb[i].name) {
		if (strcmp(info.dir, dircb[i].name) == 0) {
			dircb[i].readdir(&info, buf, filler, offset, fi);
			goto finish;
		}
		i++;
	}

	return -ENOENT;

finish:
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	if (strcmp(path, "/") == 0) {
		filler(buf, "README", NULL, 0);
	}

	return 0;
}

static int ga_files(struct path_info *info, struct stat *stbuf)
{
	cmyth_conn_t control;
	cmyth_proglist_t list;
	int count;
	int i;

	pthread_mutex_lock(&mutex);

	if ((i=lookup_server(info->host)) < 0) {
		pthread_mutex_unlock(&mutex);
		return -ENOENT;
	}

	control = ref_hold(conn[i].control);

	if (conn[i].list == NULL) {
		list = cmyth_proglist_get_all_recorded(control);
		conn[i].list = list;
		parse_progs(conn+i);
	} else {
		list = conn[i].list;
	}

	list = ref_hold(list);

	pthread_mutex_unlock(&mutex);

	stbuf->st_mode = S_IFREG | 0444;
	stbuf->st_nlink = 1;

	count = cmyth_proglist_get_count(list);

	debug("%s(): file '%s'\n", __FUNCTION__, info->file);

	for (i=0; i<count; i++) {
		cmyth_proginfo_t prog;
		long long len;
		char *pn;

		prog = cmyth_proglist_get_item(list, i);
		pn = cmyth_proginfo_pathname(prog);

		if (strcmp(pn+1, info->file) == 0) {
			cmyth_timestamp_t ts;
			time_t t;
			len = cmyth_proginfo_length(prog);
			debug("%s(): file '%s' len %lld\n",
			      __FUNCTION__, pn+1, len);
			stbuf->st_size = len;
			stbuf->st_blksize = MAX_BSIZE;
			stbuf->st_blocks = len / MAX_BSIZE;
			if ((len * MAX_BSIZE) != stbuf->st_blocks) {
				stbuf->st_blocks++;
			}
			ts = cmyth_proginfo_rec_end(prog);
			t = cmyth_timestamp_to_unixtime(ts);
			stbuf->st_atime = t;
			stbuf->st_mtime = t;
			stbuf->st_ctime = t;
			ref_release(prog);
			ref_release(pn);
			ref_release(ts);
			ref_release(control);
			ref_release(list);
			return 0;
		}
		ref_release(prog);
		ref_release(pn);
	}

	ref_release(control);
	ref_release(list);

	return -ENOENT;
}

static int ga_all(struct path_info *info, struct stat *stbuf)
{
	cmyth_conn_t control;
	cmyth_proglist_t list;
	int count;
	int i;
	struct myth_conn *mc;

	pthread_mutex_lock(&mutex);

	if ((i=lookup_server(info->host)) < 0) {
		pthread_mutex_unlock(&mutex);
		return -ENOENT;
	}

	control = ref_hold(conn[i].control);

	if (conn[i].list == NULL) {
		list = cmyth_proglist_get_all_recorded(control);
		conn[i].list = list;
		parse_progs(conn+i);
	} else {
		list = conn[i].list;
	}

	mc = conn+i;

	list = ref_hold(list);

	pthread_mutex_unlock(&mutex);

	stbuf->st_mode = S_IFLNK | 0444;
	stbuf->st_nlink = 1;

	count = cmyth_proglist_get_count(list);

	debug("%s(): file '%s'\n", __FUNCTION__, info->file);

	for (i=0; i<count; i++) {
		cmyth_proginfo_t prog;
		long long len;
		char *title, *s;
		char tmp[512];

		prog = cmyth_proglist_get_item(list, i);
		title = cmyth_proginfo_title(prog);
		s = cmyth_proginfo_subtitle(prog);

		if (mc->progs[i].suffix > 0) {
			snprintf(tmp, sizeof(tmp), "%s - %s (%d).nuv",
				 title, s, mc->progs[i].suffix);
		} else {
			snprintf(tmp, sizeof(tmp), "%s - %s.nuv", title, s);
		}

		if (strcmp(tmp, info->file) == 0) {
			cmyth_timestamp_t ts;
			time_t t;
			char *pn;

			len = cmyth_proginfo_length(prog);
			pn = cmyth_proginfo_pathname(prog);
			debug("%s(): file '%s' len %lld\n",
			      __FUNCTION__, tmp, len);
			stbuf->st_size = strlen(pn);
			ts = cmyth_proginfo_rec_end(prog);
			t = cmyth_timestamp_to_unixtime(ts);
			stbuf->st_atime = t;
			stbuf->st_mtime = t;
			stbuf->st_ctime = t;
			ref_release(pn);
			ref_release(prog);
			ref_release(ts);
			ref_release(title);
			ref_release(s);
			ref_release(control);
			ref_release(list);
			return 0;
		}
		ref_release(prog);
		ref_release(title);
		ref_release(s);
	}

	ref_release(control);
	ref_release(list);

	return -ENOENT;
}

static int readme_getattr(const char *path, struct stat *stbuf)
{
	stbuf->st_mode = S_IFREG | 0444;
	stbuf->st_nlink = 2;
	stbuf->st_size = strlen(README);
	stbuf->st_atime = readme_atime;
	stbuf->st_mtime = readme_time;
	stbuf->st_ctime = readme_time;

	return 0;
}

static int myth_getattr(const char *path, struct stat *stbuf)
{
	struct path_info info;
	int i;

	debug("%s(): path '%s'\n", __FUNCTION__, path);

	if (strcmp(path, README_PATH) == 0) {
		return readme_getattr(path, stbuf);
	}

	memset(&info, 0, sizeof(info));
	if (lookup_path(path, &info) < 0) {
		return -ENOENT;
	}

        memset(stbuf, 0, sizeof(struct stat));
        if (info.host == NULL) {
                stbuf->st_mode = S_IFDIR | 0555;
                stbuf->st_nlink = 2;
		free_info(&info);
		return 0;
	}

	if (info.dir == NULL) {
                stbuf->st_mode = S_IFDIR | 0555;
                stbuf->st_nlink = 2;
		free_info(&info);
		return 0;
	}

	i = 0;
	while (dircb[i].name) {
		if (strcmp(info.dir, dircb[i].name) == 0) {
			break;
		}
		i++;
	}
	if (dircb[i].name == NULL) {
		free_info(&info);
		return -ENOENT;
	}

	if (info.file == NULL) {
                stbuf->st_mode = S_IFDIR | 0555;
                stbuf->st_nlink = 2;
		free_info(&info);
		return 0;
	}

	i = 0;
	while (dircb[i].name) {
		if (strcmp(info.dir, dircb[i].name) == 0) {
			return dircb[i].getattr(&info, stbuf);
		}
		i++;
	}

	free_info(&info);

	return -ENOENT;
}

static int
do_seek(int i, off_t offset, int whence)
{
	off_t pos;

	pos = cmyth_file_seek(files[i].file, offset, whence);

	return pos;
}

static int
fill_buffer(int i, char *buf, size_t size)
{
	int tot, len, n = 0;

	tot = 0;
	len = cmyth_file_request_block(files[i].file, size);
	while (tot < len) {
		n = cmyth_file_get_block(files[i].file, buf+tot, len-tot);
		if (n > 0) {
			tot += n;
		}
		if (n <= 0) {
			return -1;
		}
	}

	debug("%s(): tot %d len %d n %d size %lld\n", __FUNCTION__,
	      tot, len, n, (long long)size);

	if (len < 0) {
		return -1;
	}

	return tot;
}

static int readme_read(const char *path, char *buf, size_t size, off_t offset,
		       struct fuse_file_info *fi)
{
	size_t len = strlen(README);
	size_t n;

	n = len - offset;

	if (n > size) {
		n = size;
	}

	if (n > 0) {
		memcpy(buf, README+offset, n);
	} else {
		n = 0;
	}

	time(&readme_atime);

	return n;
}

static int myth_read(const char *path, char *buf, size_t size, off_t offset,
		     struct fuse_file_info *fi)
{
	int tot, len = 0;

	pthread_mutex_lock(&mutex);

	debug("%s(): path '%s' size %lld\n", __FUNCTION__, path,
	      (long long)size);

	if (strcmp(path, README_PATH) == 0) {
		pthread_mutex_unlock(&mutex);
		return readme_read(path, buf, size, offset, fi);
	}

	if ((int)fi->fh == -1) {
		pthread_mutex_unlock(&mutex);
		return -ENOENT;
	}

	if (files[fi->fh].offset != offset) {
		if (do_seek(fi->fh, offset, SEEK_SET) < 0) {
			goto fail;
		}
	}

	tot = 0;
	while (size > 0) {
		len = (size > MAX_BSIZE) ? MAX_BSIZE : size;
		if ((len=fill_buffer(fi->fh, buf+tot, len)) <= 0)
			break;
		size -= len;
		tot += len;
	}

	files[fi->fh].offset = offset + tot;

	debug("%s(): read %d bytes at %lld (len %d)\n", __FUNCTION__,
	      tot, (long long)offset, len);

	if (len < 0) {
		goto fail;
	}

	pthread_mutex_unlock(&mutex);

	return tot;

fail:
	debug("%s(): shutting down file connection!\n", __FUNCTION__);

	ref_release(files[fi->fh].file);
	memset(files+fi->fh, 0, sizeof(files[0]));
	pthread_mutex_unlock(&mutex);

	return -ENOENT;
}

static int myth_readlink(const char *path, char *buf, size_t size)
{
	struct path_info info;
	int n;
	int i;
	cmyth_conn_t control;
	cmyth_proglist_t list;
	int count;
	struct myth_conn *mc;

	debug("%s(): path '%s' size %lld\n", __FUNCTION__, path,
	      (long long)size);

	memset(&info, 0, sizeof(info));
	if (lookup_path(path, &info) < 0) {
		return -ENOENT;
	}

	if (strcmp(info.dir, "all") != 0) {
		free_info(&info);
		return -ENOENT;
	}

	pthread_mutex_lock(&mutex);

	if ((i=lookup_server(info.host)) < 0) {
		free_info(&info);
		pthread_mutex_unlock(&mutex);
		return -ENOENT;
	}

	control = ref_hold(conn[i].control);

	if (conn[i].list == NULL) {
		list = cmyth_proglist_get_all_recorded(control);
		conn[i].list = list;
		parse_progs(conn+i);
	} else {
		list = conn[i].list;
	}

	mc = conn+i;

	list = ref_hold(list);

	pthread_mutex_unlock(&mutex);

	count = cmyth_proglist_get_count(list);

	for (i=0; i<count; i++) {
		cmyth_proginfo_t prog;
		char tmp[512];
		char *t, *s, *pn;

		prog = cmyth_proglist_get_item(list, i);
		t = cmyth_proginfo_title(prog);
		s = cmyth_proginfo_subtitle(prog);
		pn = cmyth_proginfo_pathname(prog);

		if (mc->progs[i].suffix == 0) {
			snprintf(tmp, sizeof(tmp), "%s - %s.nuv", t, s);
		} else {
			snprintf(tmp, sizeof(tmp), "%s - %s (%d).nuv", t, s,
				 mc->progs[i].suffix);
		}

		if (strcmp(tmp, info.file) == 0) {
			snprintf(tmp, sizeof(tmp), "../files%s", pn);

			memset(buf, 0, size);

			n = (strlen(tmp) > size) ? size : strlen(tmp);
			strncpy(buf, tmp, n);

			debug("%s(): link '%s' %d bytes\n", __FUNCTION__,
			      tmp, n);

			ref_release(t);
			ref_release(s);
			ref_release(pn);
			ref_release(prog);
			ref_release(control);
			ref_release(list);

			free_info(&info);

			return 0;
		}

		ref_release(t);
		ref_release(s);
		ref_release(pn);
		ref_release(prog);
	}

	ref_release(control);
	ref_release(list);

	free_info(&info);

	return -ENOENT;
}
 
static struct fuse_operations myth_oper = {
	.init = myth_init,
	.destroy = myth_destroy,
	.open = myth_open,
	.release = myth_release,
	.readdir = myth_readdir,
	.getattr = myth_getattr,
	.read = myth_read,
	.readlink = myth_readlink,
};

static void
print_help(char *prog)
{
	printf("Usage: %s [-dh] <mountpoint>\n", prog);
}

int
main(int argc, char **argv)
{
	int c;
	int opt_index;
	char *fuse[3] = { NULL, NULL, NULL };

	while ((c=getopt_long(argc, argv, "dh", opts, &opt_index)) != -1) {
		switch (c) {
		case 'd':
			F = fopen("debug.fuse", "w+");
			break;
		case 'h':
			print_help(argv[0]);
			exit(0);
			break;
		}
	}

	if (optind == argc) {
		print_help(argv[0]);
		exit(1);
	}

	fuse[0] = argv[0];
	fuse[1] = argv[optind];

	fuse_main(2, fuse, &myth_oper, NULL);

	return 0;
}
