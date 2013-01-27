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
cmyth_channel_destroy(cmyth_channel_t chan)
{
	ref_release(chan->channel_name);
	ref_release(chan->channel_sign);
	ref_release(chan->channel_string);
	ref_release(chan->channel_icon);
}

cmyth_channel_t
cmyth_channel_create(long id, char *name, char *sign, char *string, char *icon)
{
	cmyth_channel_t channel;

	channel = ref_alloc(sizeof(*channel));

	channel->channel_id = id;
	channel->channel_name = ref_strdup(name);
	channel->channel_sign = ref_strdup(sign);
	channel->channel_string = ref_strdup(string);
	channel->channel_icon = ref_strdup(icon);

	ref_set_destroy(channel, (ref_destroy_t)cmyth_channel_destroy);

	return channel;
}

long
cmyth_channel_id(cmyth_channel_t chan)
{
	return chan->channel_id;
}

char*
cmyth_channel_name(cmyth_channel_t chan)
{
	return ref_hold(chan->channel_name);
}

char*
cmyth_channel_sign(cmyth_channel_t chan)
{
	return ref_hold(chan->channel_sign);
}

char*
cmyth_channel_string(cmyth_channel_t chan)
{
	return ref_hold(chan->channel_string);
}

char*
cmyth_channel_icon(cmyth_channel_t chan)
{
	return ref_hold(chan->channel_icon);
}
