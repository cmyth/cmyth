/*
 *  Copyright (C) 2010-2012, Jon Gettler
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <limits.h>

#include "cmyth/cmyth.h"
#include "refmem/refmem.h"

#define error(msg)	fprintf(stderr, "Error: %s\n", msg)

#define MAX_BSIZE	(128*1024)

static cmyth_conn_t control;
static int tcp_control = 4096;
static int tcp_program = 128*1024;

static struct option opts[] = {
	{ "help", no_argument, 0, 'h' },
	{ "thumbnail", no_argument, 0, 't' },
	{ 0, 0, 0, 0 }
};

static void
print_help(char *prog)
{
	printf("Usage: %s [options] <backend> <filename>\n", prog);
	printf("\t-h          print this help\n");
	printf("\t-t          get the recording thumbnail\n");
}

static int
is_alive(char *host)
{

	if ((control=cmyth_conn_connect_ctrl(host, 6543,
					     16*1024, 4096)) == NULL) {
		return 0;
	}

	return 1;
}

static int
fill_buffer(cmyth_file_t file, char *buf, size_t size)
{
	int tot, len, n = 0;

	tot = 0;
	len = cmyth_file_request_block(file, size);
	while (tot < len) {
		n = cmyth_file_get_block(file, buf+tot, len-tot);
		if (n > 0) {
			tot += n;
		}
		if (n < 0) {
			return -1;
		}
	}

	if (len < 0) {
		return -1;
	}

	return tot;
}

static int
write_buffer(int fd, char *buf, int len)
{
	int tot = 0, n;

	while (tot < len) {
		n = write(fd, buf+tot, len-tot);
		if (n > 0) {
			tot += n;
		}
		if (n <= 0) {
			return -1;
		}
	}

	if (len < 0) {
		return -1;
	}

	return 0;
}

static int
dump_prog(cmyth_proginfo_t prog, int thumbnail)
{
	cmyth_conn_t c = NULL;
	cmyth_file_t f = NULL;
	char *host;
	long long len, cur;
	int port;
	int fd;

	if ((host=cmyth_proginfo_host(prog)) == NULL) {
		error("Invalid host!");
		return -1;
	}

	port = cmyth_proginfo_port(prog);

	if ((c=cmyth_conn_connect_ctrl(host, port, 16*1024,
				       tcp_control)) == NULL) {
		error("Could not connect to host!");
		return -1;
	}

	if (thumbnail) {
		if ((f=cmyth_conn_connect_thumbnail(prog, c, MAX_BSIZE,
						    tcp_program)) == NULL) {
			error("Could not open file!");
			return -1;
		}
	} else {
		if ((f=cmyth_conn_connect_file(prog, c, MAX_BSIZE,
					       tcp_program)) == NULL) {
			error("Could not open file!");
			return -1;
		}
	}

	ref_release(host);
	ref_release(c);

	if (thumbnail) {
		/* The size of the thumbnail image is unknown. */
		len = INT_MAX;
	} else {
		len = cmyth_proginfo_length(prog);
	}

	fd = fileno(stdout);

	cur = 0;
	while (cur < len) {
		char buf[MAX_BSIZE];
		int n;

		if (cmyth_file_seek(f, cur, SEEK_SET) != cur) {
			break;
		}

		n = fill_buffer(f, buf, sizeof(buf));

		if (n <= 0) {
			break;
		}

		if (write_buffer(fd, buf, n) != 0) {
			break;
		}

		cur += n;
	}

	ref_release(f);

	if (cur == len) {
		return 0;
	} else {
		if (thumbnail && (cur > 0)) {
			return 0;
		} else {
			error("Failed to read file!");
			return -1;
		}
	}
}

static int
cat_file(char *file, int thumbnail)
{
	cmyth_proglist_t episodes;
	int count, i;
	int rc = -2;

	episodes = cmyth_proglist_get_all_recorded(control);

	if (episodes == NULL) {
		error("No recordings found!");
		return -1;
	}

	count = cmyth_proglist_get_count(episodes);

	for (i=0; i<count; i++) {
		cmyth_proginfo_t prog;
		char *pathname;

		prog = cmyth_proglist_get_item(episodes, i);

		pathname = cmyth_proginfo_pathname(prog);

		if (pathname[0] == '/') {
			if (strcmp(file, pathname+1) == 0) {
				rc = dump_prog(prog, thumbnail);
				break;
			}
		} else {
			if (strcmp(file, pathname) == 0) {
				rc = dump_prog(prog, thumbnail);
				break;
			}
		}

		ref_release(pathname);
		ref_release(prog);
	}

	ref_release(episodes);

	if (rc == -2) {
		error("Recording not found!");
	}

	return rc;
}

int
main(int argc, char **argv)
{
	int c, opt_index;
	char *server, *file;
	int thumbnail = 0;

	while ((c=getopt_long(argc, argv, "ht", opts, &opt_index)) != -1) {
		switch (c) {
		case 'h':
			print_help(argv[0]);
			exit(0);
			break;
		case 't':
			thumbnail = 1;
			break;
		default:
			print_help(argv[0]);
			exit(1);
			break;
		}
	}

	if (optind == argc) {
		fprintf(stderr, "no server given!\n");
		return -1;
	}

	server = argv[optind++];
	file = argv[optind++];

	if (!is_alive(server)) {
		fprintf(stderr, "%s is not responding.\n", server);
		return -1;
	}

	if (file == NULL) {
		fprintf(stderr, "no file given\n");
		return -1;
	}

	if (cat_file(file, thumbnail) != 0) {
		return -1;
	}

	ref_release(control);

	return 0;
}
