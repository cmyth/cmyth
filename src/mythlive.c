/*
 *  Copyright (C) 2013, Jon Gettler
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
#include <inttypes.h>
#include <string.h>
#include <fcntl.h>

#include "cmyth/cmyth.h"
#include "refmem/refmem.h"

#define TSIZE	128*1024
static char transfer[TSIZE];

static struct option opts[] = {
	{ "channel", required_argument, 0, 'c' },
	{ "help", no_argument, 0, 'h' },
	{ "megabytes", required_argument, 0, 'm' },
	{ "number", required_argument, 0, 'n' },
	{ "verbose", no_argument, 0, 'v' },
	{ 0, 0, 0, 0 }
};

static void
print_help(char *prog)
{
	printf("Usage: %s [options] <backend>\n", prog);
}

static int
livetv_capture(cmyth_recorder_t rec, char *file, int bytes)
{
	int fd, len;
	int n = 0;
	int rc = -1;
	struct timeval to;

	printf("Capturing to %s\n", file);

	fd = open(file, O_WRONLY|O_TRUNC|O_CREAT, 0644);

	while (n < bytes) {
		len = cmyth_livetv_request_block(rec, sizeof(transfer));

		if (len < 0) {
			fprintf(stderr, "cmyth_livetv_request_block() failed!\n");
			goto out;
		}

		to.tv_sec = 5;
		to.tv_usec = 0;

		if (cmyth_livetv_select(rec, &to) == 0) {
			fprintf(stderr, "no data to read...\n");
			break;
		}

		len = cmyth_livetv_get_block(rec, transfer, sizeof(transfer));
		write(fd, transfer, len);
		n += len;
	}

	rc = 0;

out:
	if (fd >= 0) {
		close(fd);
	}

	return rc;
}

static int
get_livetv(cmyth_conn_t control, int channels, char *channel, int mb)
{
	cmyth_recorder_t rec;
	int rc = -1;
	int i;
	int fd = -1;

	rec = cmyth_conn_get_free_recorder(control);

	if (rec == NULL) {
		return -1;
	}

	if (cmyth_livetv_start(rec) != 0) {
		fprintf(stderr, "cmyth_livetv_start() failed!\n");
		goto out;
	}

	if (channel) {
		if (cmyth_livetv_set_channel(rec, channel) < 0) {
			fprintf(stderr, "cmyth_livetv_set_channel() failed!\n");
			goto out;
		}
	}

	for (i=0; i<channels; i++) {
		char filename[64];
		cmyth_proginfo_t prog;

		prog = cmyth_recorder_get_cur_proginfo(rec);

		if (prog) {
			char *channame = cmyth_proginfo_channame(prog);
			char *name = strdup(channame);
			char *p;

			while ((p=strchr(name, ' ')) != NULL) {
				*p = '_';
			}

			snprintf(filename, sizeof(filename),
				 "livetv_%.2d-%s.mpg", i, name);

			ref_release(channame);
		} else {
			snprintf(filename, sizeof(filename),
				 "livetv_%.2d.mpg", i);
		}

		if (livetv_capture(rec, filename, mb) < 0) {
			fprintf(stderr, "livetv_capture() failed!\n");
			goto out;
		}

		if (i == (channels-1)) {
			if (cmyth_livetv_stop(rec) < 0) {
				fprintf(stderr, "stopping live TV failed!\n");
			}
		} else {
			if (cmyth_livetv_change_channel(rec,
							CHANNEL_DIRECTION_UP) < 0) {
				fprintf(stderr, "change channel failed!\n");
				goto out;
			}
		}

		if (prog) {
			if (cmyth_proginfo_delete_recording(control,
							    prog) < 0) {
				fprintf(stderr,
					"deleting live TV recording failed!\n");
			}

			ref_release(prog);
		}
	}

	rc = 0;

out:
	if (fd >= 0) {
		close(fd);
	}

	ref_release(rec);

	return rc;
}

int
main(int argc, char **argv)
{
	int c, opt_index;
	int verbose = 0;
	char *server;
	cmyth_conn_t control;
	int n = 1;
	char *channel = NULL;
	int mb = 4194304;

	while ((c=getopt_long(argc, argv, "c:hm:n:v",
			      opts, &opt_index)) != -1) {
		switch (c) {
		case 'c':
			channel = strdup(optarg);
			break;
		case 'h':
			print_help(argv[0]);
			exit(0);
			break;
		case 'm':
			mb = atoi(optarg);
			break;
		case 'n':
			n = atoi(optarg);
			break;
		case 'v':
			verbose++;
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

	server = argv[optind];

	if ((control=cmyth_conn_connect_ctrl(server, 6543,
					     16*1024, 4096)) == NULL) {
		fprintf(stderr, "connection failed!\n");
		return -1;
	}

	if (get_livetv(control, n, channel, mb) < 0) {
		fprintf(stderr, "livetv failed!\n");
		return -1;
	}

	ref_release(control);

	return 0;
}
