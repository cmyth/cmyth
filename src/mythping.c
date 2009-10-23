/*
 *  Copyright (C) 2009, Jon Gettler
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

#include "cmyth/cmyth.h"
#include "refmem/refmem.h"

static cmyth_conn_t control;

static struct option opts[] = {
	{ "help", no_argument, 0, 'h' },
	{ "verbose", no_argument, 0, 'v' },
	{ 0, 0, 0, 0 }
};

static void
print_help(char *prog)
{
	printf("Usage: %s [-v] <backend>\n", prog);
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

int
main(int argc, char **argv)
{
	int c, opt_index;
	int verbose = 0;
	char *server;

	while ((c=getopt_long(argc, argv, "hv", opts, &opt_index)) != -1) {
		switch (c) {
		case 'h':
			print_help(argv[0]);
			exit(0);
			break;
		case 'v':
			verbose = 1;
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

	if (!is_alive(server)) {
		printf("%s is not responding.\n", server);
		return 1;
	}

	printf("%s is alive.\n", server);

	if (verbose) {
		int version, count;
		cmyth_proglist_t list;

		version = cmyth_conn_get_protocol_version(control);

		printf("\tprotocol version: %d\n", version);

		list = cmyth_proglist_get_all_recorded(control);
		count = cmyth_proglist_get_count(list);

		printf("\trecordings: %d\n", count);

		ref_release(list);
	}

	ref_release(control);

	return 0;
}
