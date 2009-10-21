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

#include "cmyth/cmyth.h"
#include "refmem/refmem.h"

static int
is_alive(char *host)
{
	cmyth_conn_t control;

	if ((control=cmyth_conn_connect_ctrl(host, 6543,
					     16*1024, 4096)) == NULL) {
		return 0;
	}
	ref_release(control);

	return 1;
}

int
main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "no server given!\n");
		return -1;
	}

	if (is_alive(argv[1])) {
		printf("%s is alive.\n", argv[1]);
		return 0;
	} else {
		printf("%s is not responding.\n", argv[1]);
		return 1;
	}
}
