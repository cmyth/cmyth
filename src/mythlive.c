/*
 *  Copyright (C) 2013-2014, Jon Gettler
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

#define TSIZE	(128*1024)
static char transfer[TSIZE];

static int verbose = 0;

static struct option opts[] = {
	{ "channel", required_argument, 0, 'c' },
	{ "help", no_argument, 0, 'h' },
	{ "megabytes", required_argument, 0, 'm' },
	{ "number", required_argument, 0, 'n' },
	{ "random", no_argument, 0, 'r' },
	{ "seconds", required_argument, 0, 's' },
	{ "verbose", no_argument, 0, 'v' },
	{ 0, 0, 0, 0 }
};

static void
print_help(char *prog)
{
	printf("Usage: %s [options] <backend>\n", prog);
	printf("       --channel <name>     channel to record\n");
	printf("       --help               print this help\n");
	printf("       --megabytes <num>    megabytes to record\n");
	printf("       --number <num>       number of channels to record\n");
	printf("       --random             random channel changes\n");
	printf("       --seconds <num>      seconds to record\n");
	printf("       --verbose            verbose output\n");
}

static int
livetv_capture(cmyth_recorder_t rec, char *file, int mb, int seconds)
{
	int fd, len;
	int n = 0;
	int rc = -1;
	struct timeval to;
	time_t end = 0;
	cmyth_proginfo_t prog1, prog2;
	int bytes = mb*1024*1024;
	char *path;

	printf("Capturing to %s\n", file);

	prog1 = cmyth_recorder_get_cur_proginfo(rec);

	if (verbose > 3) {
		path = cmyth_proginfo_pathname(prog1);
		printf("  pathname %s\n", path);
		ref_release(path);
	}

	if (seconds > 0) {
		end = time(NULL) + seconds;
	}

	fd = open(file, O_WRONLY|O_TRUNC|O_CREAT, 0644);

	while (1) {
		if (end == 0) {
			if (n < bytes) {
				break;
			}
		} else {
			if (time(NULL) >= end) {
				break;
			}
		}

		len = cmyth_livetv_request_block(rec, sizeof(transfer));

		if (len < 0) {
			fprintf(stderr,
				"cmyth_livetv_request_block() failed!\n");
			goto out;
		}

		if (len == 0) {
			fprintf(stderr, "no data to read...retry\n");
			continue;
		}

		to.tv_sec = 5;
		to.tv_usec = 0;

		if (cmyth_livetv_select(rec, &to) == 0) {
			fprintf(stderr, "no data to read...abort\n");
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

	prog2 = cmyth_recorder_get_cur_proginfo(rec);

	if (verbose > 3) {
		path = cmyth_proginfo_pathname(prog2);
		printf("  pathname %s\n", path);
		ref_release(path);
	}

	if (verbose > 2) {
		char *title, *chan;

		title = cmyth_proginfo_title(prog1);
		chan = cmyth_proginfo_channame(prog1);
		printf("  prog started as '%s' on '%s'\n", title, chan);
		ref_release(title);
		ref_release(chan);

		title = cmyth_proginfo_title(prog2);
		chan = cmyth_proginfo_channame(prog2);
		printf("  prog ended as '%s' on '%s'\n", title, chan);
		ref_release(title);
		ref_release(chan);
	}

	ref_release(prog1);
	ref_release(prog2);

	return rc;
}

static int
next_channel(cmyth_recorder_t rec, cmyth_chanlist_t cl, int random)
{
	int rc;
	cmyth_proginfo_t prog1, prog2;
	char *chan1, *chan2;

	prog1 = cmyth_recorder_get_cur_proginfo(rec);

	if (random) {
		cmyth_channel_t chan;
		int count = cmyth_chanlist_get_count(cl);
		int r;
		char *name;

#if defined(HAS_ARC4RANDOM)
		r = arc4random_uniform(count);
#else
		r = rand() % count;
#endif

		chan = cmyth_chanlist_get_item(cl, r);
		name = cmyth_channel_name(chan);

		rc = cmyth_livetv_set_channel(rec, name);

		ref_release(name);
		ref_release(chan);
	} else {
		rc = cmyth_livetv_change_channel(rec, CHANNEL_DIRECTION_UP);
	}

	prog2 = cmyth_recorder_get_cur_proginfo(rec);

	if (cmyth_proginfo_compare(prog1, prog2) == 0) {
		printf("%s(): program has not changed!\n", __FUNCTION__);
	}

	chan1 = cmyth_proginfo_channame(prog1);
	chan2 = cmyth_proginfo_channame(prog2);

	if (strcmp(chan1, chan2) == 0) {
		printf("%s(): channel has not changed!\n", __FUNCTION__);
	}

	ref_release(chan1);
	ref_release(chan2);
	ref_release(prog1);
	ref_release(prog2);

	return rc;
}

static int
get_livetv(cmyth_conn_t control, int channels, char *channel,
	   int mb, int seconds, int random)
{
	cmyth_recorder_t rec;
	cmyth_chain_t chain;
	cmyth_proginfo_t prog;
	cmyth_chanlist_t cl;
	int rc = -1;
	int i;
	int fd = -1;

	rec = cmyth_conn_get_free_recorder(control);

	if (rec == NULL) {
		return -1;
	}

	cl = cmyth_recorder_get_chanlist(rec);

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

		prog = cmyth_recorder_get_cur_proginfo(rec);

		if (prog) {
			char *channame = cmyth_proginfo_channame(prog);
			char *p;

			while ((p=strchr(channame, ' ')) != NULL) {
				*p = '_';
			}

			snprintf(filename, sizeof(filename),
				 "livetv_%.2d-%s.mpg", i, channame);

			ref_release(channame);
		} else {
			snprintf(filename, sizeof(filename),
				 "livetv_%.2d.mpg", i);
		}

		if (livetv_capture(rec, filename, mb, seconds) < 0) {
			fprintf(stderr, "livetv_capture() failed!\n");
			goto out;
		}

		if (i < (channels-1)) {
			if (next_channel(rec, cl, random) < 0) {
				fprintf(stderr, "change channel failed!\n");
				goto out;
			}
		}

		ref_release(prog);
	}

	if (cmyth_livetv_stop(rec) < 0) {
		fprintf(stderr, "stopping live TV failed!\n");
	}

	chain = cmyth_livetv_get_chain(rec);

	if (chain) {
		for (i=0; i<cmyth_chain_get_count(chain); i++) {
			prog = cmyth_chain_get_prog(chain, i);
			if (prog) {
				char *path;

				if (verbose > 2) {
					path = cmyth_proginfo_pathname(prog);
					printf("delete prog %s\n", path);
					ref_release(path);
				}

				cmyth_proginfo_delete_recording(control, prog);

				ref_release(prog);
			}
		}

		ref_release(chain);
	}

	rc = 0;

out:
	if (fd >= 0) {
		close(fd);
	}

	ref_release(cl);
	ref_release(rec);

	return rc;
}

int
main(int argc, char **argv)
{
	int c, opt_index;
	char *server;
	cmyth_conn_t control;
	int n = 1;
	char *channel = NULL;
	int mb = 32;
	int seconds = 0;
	int rc = 0;
	int random = 0;

	while ((c=getopt_long(argc, argv, "c:hm:n:rs:v",
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
		case 'r':
			random = 1;
			break;
		case 's':
			seconds = atoi(optarg);
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

	if (get_livetv(control, n, channel, mb, seconds, random) < 0) {
		fprintf(stderr, "livetv failed!\n");
		rc = -1;
	}

	ref_release(control);

	if (verbose > 1) {
		unsigned int refs, bytes;

		ref_get_usage(&refs, &bytes);

		printf("Refmem: refs  %d\n", refs);
		printf("Refmem: bytes %d\n", bytes);

		if (refs > 0) {
			ref_alloc_show();
		}
	}

	return rc;
}
