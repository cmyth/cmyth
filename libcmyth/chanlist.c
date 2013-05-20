/*
 *  Copyright (C) 2013, Jon Gettler
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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <cmyth_local.h>

static void
cmyth_chanlist_destroy(cmyth_chanlist_t list)
{
	int i;

	if (!list) {
		return;
	}

	for (i=0; i<list->chanlist_count; i++) {
		if (list->chanlist_list[i]) {
			ref_release(list->chanlist_list[i]);
		}
		list->chanlist_list[i] = NULL;
	}
	if (list->chanlist_list) {
		ref_release(list->chanlist_list);
	}
}

cmyth_chanlist_t
cmyth_chanlist_create(void)
{
	cmyth_chanlist_t ret;
	int max = 16;

	ret = ref_alloc(sizeof(*ret));
	if (!ret) {
		return(NULL);
	}
	ref_set_destroy(ret, (ref_destroy_t)cmyth_chanlist_destroy);

	ret->chanlist_list = ref_alloc(sizeof(cmyth_channel_t*)*max);
	ret->chanlist_count = 0;
	ret->chanlist_max = max;

	return ret;
}

cmyth_channel_t
cmyth_chanlist_get_item(cmyth_chanlist_t list, unsigned int index)
{
	if (!list) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL channel list\n",
			  __FUNCTION__);
		return NULL;
	}

	if (!list->chanlist_list) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL list\n",
			  __FUNCTION__);
		return NULL;
	}

	if (index >= (unsigned int)list->chanlist_count) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: index %d out of range\n",
			  __FUNCTION__, index);
		return NULL;
	}

	return ref_hold(list->chanlist_list[index]);
}

int
cmyth_chanlist_get_count(cmyth_chanlist_t list)
{
	if (!list) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: NULL channel list\n",
			  __FUNCTION__);
		return -EINVAL;
	}
	return list->chanlist_count;
}

int
cmyth_chanlist_add(cmyth_chanlist_t list, cmyth_channel_t channel)
{
	if (list->chanlist_count == list->chanlist_max) {
		int max = list->chanlist_max * 2;
		cmyth_channel_t *new_list;

		new_list = ref_realloc(list->chanlist_list,
				       sizeof(cmyth_channel_t*)*max);

		if (new_list == NULL) {
			return -1;
		}

		list->chanlist_max = max;
		list->chanlist_list = new_list;
	}

	list->chanlist_list[list->chanlist_count++] = ref_hold(channel);

	return 0;
}
