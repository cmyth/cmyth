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

struct event_loop_args {
	cmyth_recorder_t rec;
	cmyth_chain_t chain;
};

static void *cmyth_chain_event_loop(void *data);

static void
cmyth_chain_destroy(cmyth_chain_t chain)
{
	unsigned int i;

	if (chain == NULL) {
		return;
	}

	if (chain->chain_thread) {
		pthread_cancel(chain->chain_thread);
		pthread_join(chain->chain_thread, NULL);
		chain->chain_thread = 0;
	}

	pthread_mutex_lock(&chain->chain_mutex);

	if (chain->chain_id) {
		ref_release(chain->chain_id);
		chain->chain_id = NULL;
	}

	for (i=0; i<chain->chain_count; i++) {
		cmyth_chain_entry_t entry;

		if ((entry=chain->chain_list[i]) != NULL) {
			ref_release(entry->prog);
			ref_release(entry->file);
			entry->prog = NULL;
			entry->file = NULL;
		}

		ref_release(entry);

		chain->chain_list[i] = NULL;
	}

	if (chain->chain_list) {
		ref_release(chain->chain_list);
		chain->chain_list = NULL;
	}

	if (chain->chain_thread_args) {
		struct event_loop_args *data =
			(struct event_loop_args*)chain->chain_thread_args;
		ref_release(data);
		chain->chain_thread_args = NULL;
	}

	if (chain->chain_thread_rec) {
		ref_release(chain->chain_thread_rec);
		chain->chain_thread_rec = NULL;
	}

	if (chain->chain_event) {
		ref_release(chain->chain_event);
		chain->chain_event = NULL;
	}

	if (chain->chain_conn) {
		ref_release(chain->chain_conn);
		chain->chain_conn = NULL;
	}

	pthread_mutex_destroy(&chain->chain_mutex);
	pthread_cond_destroy(&chain->chain_cond);
}

cmyth_chain_t
cmyth_chain_create(cmyth_recorder_t rec, char *chain_id)
{
	cmyth_chain_t chain;
	struct event_loop_args *args;

	args = ref_alloc(sizeof(*args));

	chain = ref_alloc(sizeof(*chain));

	chain->chain_id = ref_strdup(chain_id);
	chain->chain_count = 0;
	chain->chain_current = -1;
	chain->chain_list = NULL;
	chain->chain_callback = NULL;
	chain->chain_event = NULL;
	chain->chain_thread = 0;
	chain->chain_conn = ref_hold(rec->rec_conn);

	pthread_mutex_init(&chain->chain_mutex, NULL);
	pthread_cond_init(&chain->chain_cond, NULL);

	ref_set_destroy(chain, (ref_destroy_t)cmyth_chain_destroy);

	args->rec = ref_hold(rec);
	args->chain = ref_hold(chain);

	chain->chain_thread_args = (void*)args;

	pthread_create(&chain->chain_thread, NULL,
		       cmyth_chain_event_loop, (void*)args);

	return chain;
}

cmyth_chain_t
cmyth_livetv_get_chain(cmyth_recorder_t rec)
{
	if (rec == NULL) {
		return NULL;
	}

	return ref_hold(rec->rec_chain);
}

int
cmyth_chain_set_current(cmyth_chain_t chain, cmyth_proginfo_t prog)
{
	unsigned int i;
	int rc = -1;
	void (*callback)(cmyth_proginfo_t prog) = NULL;
	cmyth_proginfo_t cb_prog = NULL;

	if ((chain == NULL) || (prog == NULL)) {
		return -1;
	}

	pthread_mutex_lock(&chain->chain_mutex);

	for (i=0; i<chain->chain_count; i++) {
		if (cmyth_proginfo_compare(prog,
					   chain->chain_list[i]->prog) == 0) {
			chain->chain_current = i;
			callback = chain->chain_callback;
			if (callback) {
				cb_prog = ref_hold(chain->chain_list[i]->prog);
			}
			break;
		}
	}

	pthread_mutex_unlock(&chain->chain_mutex);

	if (callback && cb_prog) {
		callback(cb_prog);
		ref_release(cb_prog);
	}

	return rc;
}

int
cmyth_chain_switch_to_locked(cmyth_chain_t chain, int index)
{
	int rc = -1;

	if ((index >= 0) && (index < (int)chain->chain_count)) {
		cmyth_conn_t conn;
		cmyth_file_t file;
		cmyth_proginfo_t prog;
		char *path, *title;

		if (chain->chain_list[index]->file != NULL) {
			chain->chain_current = index;
			return 0;
		}

		prog = chain->chain_list[index]->prog;

		if (prog == NULL) {
			return -1;
		}

		conn = cmyth_conn_connect_ctrl(prog->proginfo_hostname,
					       prog->proginfo_port,
					       16*1024, 4096);
		if (conn == NULL) {
			return -1;
		}

		path = cmyth_proginfo_pathname(prog);
		title = cmyth_proginfo_title(prog);
		cmyth_dbg(CMYTH_DBG_DEBUG, "%s(): connect to file %s [%s]\n",
			  __FUNCTION__, path, title);
		ref_release(path);
		ref_release(title);

		file = cmyth_conn_connect_file(prog, conn, 128*1024, 128*1024);

		if (file) {
			chain->chain_current = index;
			chain->chain_list[index]->file = file;
		}

		ref_release(conn);

		rc = 0;
	}

	return rc;
}

int
cmyth_chain_switch(cmyth_chain_t chain, int delta)
{
	int rc;

	if (chain == NULL) {
		return -1;
	}

	pthread_mutex_lock(&chain->chain_mutex);

	rc = cmyth_chain_switch_to_locked(chain, chain->chain_current + delta);

	pthread_mutex_unlock(&chain->chain_mutex);

	return rc;
}

int
cmyth_chain_switch_to(cmyth_chain_t chain, int index)
{
	int rc;

	if (chain == NULL) {
		return -1;
	}

	pthread_mutex_lock(&chain->chain_mutex);

	rc = cmyth_chain_switch_to_locked(chain, index);

	pthread_mutex_unlock(&chain->chain_mutex);

	return rc;
}

int
cmyth_chain_switch_last(cmyth_chain_t chain)
{
	int rc;

	if (chain == NULL) {
		return -1;
	}

	pthread_mutex_lock(&chain->chain_mutex);

	rc = cmyth_chain_switch_to_locked(chain, chain->chain_count - 1);

	pthread_mutex_unlock(&chain->chain_mutex);

	return rc;
}

int
cmyth_chain_get_count(cmyth_chain_t chain)
{
	int count;

	if (chain == NULL) {
		return -1;
	}

	pthread_mutex_lock(&chain->chain_mutex);

	count = chain->chain_count;

	pthread_mutex_unlock(&chain->chain_mutex);

	return count;
}

cmyth_file_t
cmyth_chain_get_file(cmyth_chain_t chain, cmyth_proginfo_t prog)
{
	cmyth_file_t file = NULL;
	unsigned int i;

	if ((chain == NULL) || (prog == NULL)) {
		return NULL;
	}

	pthread_mutex_lock(&chain->chain_mutex);

	for (i=0; i<chain->chain_count; i++) {
		if (cmyth_proginfo_compare(prog,
					   chain->chain_list[i]->prog) == 0) {
			file = ref_hold(chain->chain_list[i]->file);
			break;
		}
	}

	pthread_mutex_unlock(&chain->chain_mutex);

	return file;
}

cmyth_proginfo_t
cmyth_chain_get_prog(cmyth_chain_t chain, unsigned int which)
{
	cmyth_proginfo_t prog = NULL;

	if (chain == NULL) {
		return NULL;
	}

	pthread_mutex_lock(&chain->chain_mutex);

	if (which < chain->chain_count) {
		prog = chain->chain_list[which]->prog;
		ref_hold(prog);
	}

	pthread_mutex_unlock(&chain->chain_mutex);

	return prog;
}

cmyth_proginfo_t
cmyth_chain_get_current(cmyth_chain_t chain)
{
	cmyth_proginfo_t prog = NULL;

	if (chain == NULL) {
		return NULL;
	}

	pthread_mutex_lock(&chain->chain_mutex);

	if (chain->chain_list) {
		prog = chain->chain_list[chain->chain_current]->prog;
		ref_hold(prog);
	}

	pthread_mutex_unlock(&chain->chain_mutex);

	return prog;
}

int
cmyth_chain_remove_prog(cmyth_chain_t chain, cmyth_proginfo_t prog)
{
	return -1;
}

int
cmyth_chain_set_callback(cmyth_chain_t chain,
			 void (*callback)(cmyth_proginfo_t prog))
{
	if (chain == NULL) {
		return -1;
	}

	pthread_mutex_lock(&chain->chain_mutex);

	chain->chain_callback = callback;

	pthread_mutex_unlock(&chain->chain_mutex);

	return 0;
}

cmyth_file_t
cmyth_chain_current_file(cmyth_chain_t chain)
{
	cmyth_file_t file = NULL;

	if (chain == NULL) {
		return NULL;
	}

	pthread_mutex_lock(&chain->chain_mutex);

	if (chain->chain_list) {
		file = chain->chain_list[chain->chain_current]->file;
		if (file == NULL) {
			cmyth_chain_switch_to_locked(chain,
						     chain->chain_current);
			file = chain->chain_list[chain->chain_current]->file;
		}
		ref_hold(file);
	}

	pthread_mutex_unlock(&chain->chain_mutex);

	return file;
}

static void
cmyth_chain_update(cmyth_chain_t chain, cmyth_recorder_t rec, char *msg)
{
	char *p;
	cmyth_proginfo_t prog = NULL;
	cmyth_chain_entry_t entry;
	int size, tip;
	long long offset;
	int start = 0;
	char *path;

	if ((p=strchr(msg, ' ')) != NULL) {
		*(p++) = '\0';

		if (strcmp(msg, "LIVETV_CHAIN UPDATE") != 0) {
			return;
		}
	} else {
		p = msg;
	}

	prog = cmyth_recorder_get_cur_proginfo(rec);

	if (prog == NULL) {
		return;
	}

	path = cmyth_proginfo_pathname(prog);

	if (path == NULL) {
		return;
	}

	if (strlen(path) == 0) {
		ref_release(path);
		ref_release(prog);
		return;
	}
	ref_release(path);

	pthread_mutex_lock(&chain->chain_mutex);

	if (!chain->chain_id || (strncmp(p, chain->chain_id, strlen(p)) != 0)) {
		goto out;
	}

	tip = chain->chain_count - 1;

	if (tip >= 0) {
		path = cmyth_proginfo_pathname(chain->chain_list[tip]->prog);
		ref_release(path);

		if (cmyth_proginfo_compare(prog,
					   chain->chain_list[tip]->prog) == 0) {
			ref_release(prog);
			goto out;
		}

		offset = chain->chain_list[tip]->offset +
			cmyth_proginfo_length(chain->chain_list[tip]->prog);
	} else {
		offset = 0;
		start = 1;
	}

	size = sizeof(*chain->chain_list) * (++chain->chain_count);

	chain->chain_list = ref_realloc(chain->chain_list, size);

	entry = ref_alloc(sizeof(*entry));

	entry->prog = prog;
	entry->file = NULL;
	entry->offset = offset;

	chain->chain_list[tip+1] = entry;

	pthread_cond_broadcast(&chain->chain_cond);

out:
	pthread_mutex_unlock(&chain->chain_mutex);

	if (start) {
		chain->chain_current = 0;
		cmyth_chain_switch(chain, 0);
	}
}

static void*
cmyth_chain_event_loop(void *data)
{
	int oldstate;
	struct event_loop_args *args = (struct event_loop_args*)data;
	cmyth_chain_t chain = args->chain;
	cmyth_recorder_t rec = args->rec;
	cmyth_recorder_t new_rec;

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s(): thread started!\n", __FUNCTION__);

	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);

	chain->chain_event = cmyth_conn_connect_event(rec->rec_server,
						      rec->rec_port,
						      16*1024, 4096);

	if (chain->chain_event == NULL) {
		return NULL;
	}

	new_rec = cmyth_conn_get_recorder(rec->rec_conn, rec->rec_id);

	chain->chain_thread_rec = new_rec;

	ref_release(chain);
	ref_release(rec);

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);

	while (1) {
		cmyth_event_t next;
		char buf[256];

		next = cmyth_event_get(chain->chain_event, buf, sizeof(buf));

		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &oldstate);

		switch (next) {
		case CMYTH_EVENT_LIVETV_CHAIN_UPDATE:
			cmyth_dbg(CMYTH_DBG_DEBUG,
				  "%s(): chain update %s\n", __FUNCTION__, buf);
			cmyth_chain_update(chain, new_rec, buf);
			break;
		default:
			break;
		}

		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
	}

	return NULL;
}

void
cmyth_chain_lock(cmyth_chain_t chain)
{
	if (chain) {
		pthread_mutex_lock(&chain->chain_mutex);
	}
}

void
cmyth_chain_unlock(cmyth_chain_t chain)
{
	if (chain) {
		pthread_mutex_unlock(&chain->chain_mutex);
	}
}

void
cmyth_chain_add_wait(cmyth_chain_t chain)
{
	if (chain) {
		struct timespec to;

		to.tv_sec = 5;
		to.tv_nsec = 0;

		pthread_cond_timedwait(&chain->chain_cond, &chain->chain_mutex,
				       &to);
	}
}
