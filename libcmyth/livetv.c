/*
 *  Copyright (C) 2006-2013, Sergio Slobodrian, Jon Gettler
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

/*
 * livetv.c -     functions to handle operations on MythTV livetv chains.  A
 *                MythTV livetv chain is the part of the backend that handles
 *                recording of live-tv for streaming to a MythTV frontend.
 *                This allows the watcher to do things like pause, rewind
 *                and so forth on live-tv.
 */
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <cmyth_local.h>

/*
 * cmyth_livetv_chain_destroy(cmyth_livetv_chain_t ltc)
 * 
 * Scope: PRIVATE (static)
 *
 * Description
 *
 * Clean up and free a livetv chain structure.  This should only be done
 * by the ref_release() code.  Everyone else should call
 * ref_release() because ring buffer structures are reference
 * counted.
 *
 * Return Value:
 *
 * None.
 */
static void
cmyth_livetv_chain_destroy(cmyth_livetv_chain_t ltc)
{
	int i;

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s\n", __FUNCTION__);
	if (!ltc) {
		return;
	}

	if (ltc->chainid) {
		ref_release(ltc->chainid);
	}
	if (ltc->chain_urls) {
		for(i=0;i<ltc->chain_ct; i++)
			if (ltc->chain_urls[i])
				ref_release(ltc->chain_urls[i]);
		ref_release(ltc->chain_urls);
	}
	if (ltc->chain_files) {
		for(i=0;i<ltc->chain_ct; i++)
			if (ltc->chain_files[i])
				ref_release(ltc->chain_files[i]);
		ref_release(ltc->chain_files);
	}
	if (ltc->progs) {
		for(i=0;i<ltc->chain_ct; i++)
			if (ltc->progs[i])
				ref_release(ltc->progs[i]);
		ref_release(ltc->progs);
	}
}

/*
 * cmyth_livetv_chain_create(void)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Allocate and initialize a ring buffer structure.
 *
 * Return Value:
 *
 * Success: A non-NULL cmyth_livetv_chain_t (this type is a pointer)
 *
 * Failure: A NULL cmyth_livetv_chain_t
 */
cmyth_livetv_chain_t
cmyth_livetv_chain_create(char * chainid)
{
	cmyth_livetv_chain_t ret = ref_alloc(sizeof(*ret));

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s\n", __FUNCTION__);
	if (!ret) {
		return NULL;
	}

	ret->chainid = ref_strdup(chainid);
	ret->chain_ct = 0;
	ret->chain_switch_on_create = 0;
	ret->chain_current = -1;
	ret->chain_urls = NULL;
	ret->chain_files = NULL;
	ret->progs = NULL;
	ref_set_destroy(ret, (ref_destroy_t)cmyth_livetv_chain_destroy);
	return ret;
}

/*
 * cmyth_livetv_chain_get_block(cmyth_recorder_t rec, char *buf,
 *				unsigned long len)
 * Scope: PUBLIC
 * Description
 * Read incoming file data off the network into a buffer of length len.
 *
 * Return Value:
 * Sucess: number of bytes read into buf
 * Failure: -1
 */
int
cmyth_livetv_chain_get_block(cmyth_recorder_t rec, char *buf,
			     unsigned long len)
{
	cmyth_file_t file;
	int rc;

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s [%s:%d]: (trace) {\n",
		  __FUNCTION__, __FILE__, __LINE__);

	if (!rec || !rec->rec_connected) {
		return -EINVAL;
	}

	if ((file=cmyth_livetv_current_file(rec)) == NULL) {
		return -1;
	}

	rc = cmyth_file_get_block(file, buf, len);

	ref_release(file);

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s [%s:%d]: (trace) }\n",
		  __FUNCTION__, __FILE__, __LINE__);

	return rc;
}

static int
cmyth_livetv_chain_select(cmyth_recorder_t rec, struct timeval *timeout)
{
	cmyth_file_t file;
	int rc;

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s [%s:%d]: (trace) {\n",
		  __FUNCTION__, __FILE__, __LINE__);

	if (!rec || !rec->rec_connected) {
		return -EINVAL;
	}

	if ((file=cmyth_livetv_current_file(rec)) == NULL) {
		return -1;
	}

	rc = cmyth_file_select(file, timeout);

	ref_release(file);

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s [%s:%d]: (trace) }\n",
		  __FUNCTION__, __FILE__, __LINE__);

	return rc;
}


/*
 * cmyth_livetv_chain_switch(cmyth_recorder_t rec, int dir)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Switches to the next or previous chain depending on the
 * value of dir. Dir is usually 1 or -1.
 *
 * Return Value:
 *
 * Sucess: 1
 *
 * Failure: 0
 */
int
cmyth_livetv_chain_switch(cmyth_recorder_t rec, int dir)
{
	return cmyth_chain_switch(rec->rec_chain, dir);
}

/*
 * cmyth_livetv_chain_request_block(cmyth_recorder_t file, unsigned long len)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Request a file data block of a certain size, and return when the
 * block has been transfered.
 *
 * Return Value:
 *
 * Sucess: number of bytes transfered
 *
 * Failure: an int containing -errno
 */
static int
cmyth_livetv_chain_request_block(cmyth_recorder_t rec, unsigned long len)
{
	int rc;
	cmyth_file_t file;

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s [%s:%d]: (trace) {\n", __FUNCTION__,
				__FILE__, __LINE__);

	if (!rec) {
		cmyth_dbg(CMYTH_DBG_ERROR, "%s: no connection\n",
			  __FUNCTION__);
		return -EINVAL;
	}

	if (!rec->rec_connected) {
		return -1;
	}

	pthread_mutex_lock(&rec->rec_conn->conn_mutex);

retry:
	if ((file=cmyth_livetv_current_file(rec)) == NULL) {
		rc = -1;
		goto out;
	}

	rc = cmyth_file_request_block(file, len);

	if (rc == 0) {
		cmyth_dbg(CMYTH_DBG_DEBUG,
			  "%s(): no data, move forward in chain and retry\n",
			  __FUNCTION__);
		if (cmyth_chain_switch(rec->rec_chain, 1) == 0) {
			ref_release(file);
			goto retry;
		}
	}

	ref_release(file);

    out:
	pthread_mutex_unlock(&rec->rec_conn->conn_mutex);

	cmyth_dbg(CMYTH_DBG_DEBUG, "%s [%s:%d]: (trace) }\n",
				__FUNCTION__, __FILE__, __LINE__);

	return rc;
}


/*
 * cmyth_livetv_chain_seek(cmyth_recorder_t file, long long offset, int whence)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Seek to a new position in the file based on the value of whence:
 *	SEEK_SET
 *		The offset is set to offset bytes.
 *	SEEK_CUR
 *		The offset is set to the current position plus offset bytes.
 *	SEEK_END
 *		The offset is set to the size of the file minus offset bytes.
 *
 * Return Value:
 *
 * Sucess: 0
 *
 * Failure: an int containing -errno
 */
static long long
cmyth_livetv_chain_seek(cmyth_recorder_t rec, long long offset, int whence)
{
	long long ret;
	cmyth_file_t file;

	if (rec == NULL)
		return -EINVAL;

	if (!rec->rec_connected) {
		return -1;
	}

	pthread_mutex_lock(&rec->rec_conn->conn_mutex);

	if ((file=cmyth_livetv_current_file(rec)) == NULL) {
		ret = -1;
		goto out;
	}

	/*
	 * XXX: This needs to seek across the entire chain...
	 */

	ret = cmyth_file_seek(file, offset, whence);

	ref_release(file);

    out:
	pthread_mutex_unlock(&rec->rec_conn->conn_mutex);
	
	return ret;
}

/*
 * cmyth_livetv_seek(cmyth_recorder_t file, long long offset, int whence)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Seek to a new position in the file based on the value of whence:
 *	SEEK_SET
 *		The offset is set to offset bytes.
 *	SEEK_CUR
 *		The offset is set to the current position plus offset bytes.
 *	SEEK_END
 *		The offset is set to the size of the file minus offset bytes.
 * This function will select the appropriate call based on the protocol.
 *
 * Return Value:
 *
 * Sucess: 0
 *
 * Failure: an int containing -errno
 */
long long
cmyth_livetv_seek(cmyth_recorder_t rec, long long offset, int whence)
{
	long long rtrn;

	if (!rec->rec_connected) {
		return -1;
	}

	if (rec->rec_conn->conn_version >= 26) {
		rtrn = cmyth_livetv_chain_seek(rec, offset, whence);
	} else {
		rtrn = cmyth_ringbuf_seek(rec, offset, whence);
	}

	return rtrn;
}

/*
 * cmyth_livetv_request_block(cmyth_recorder_t file, unsigned long len)
 * 
 * Scope: PUBLIC
 *
 * Description
 *
 * Request a file data block of a certain size, and return when the
 * block has been transfered. This function will select the appropriate
 * call to use based on the protocol.
 *
 * Return Value:
 *
 * Sucess: number of bytes transfered
 *
 * Failure: an int containing -errno
 */
int
cmyth_livetv_request_block(cmyth_recorder_t rec, unsigned long size)
{
	unsigned long rtrn;

	if (!rec->rec_connected) {
		return -1;
	}

	if (rec->rec_conn->conn_version >= 26) {
		rtrn = cmyth_livetv_chain_request_block(rec, size);
	} else {
		rtrn = cmyth_ringbuf_request_block(rec, size);
	}

	return rtrn;
}

int
cmyth_livetv_select(cmyth_recorder_t rec, struct timeval *timeout)
{
	int rtrn;
	
	if (!rec->rec_connected) {
		return -1;
	}

	if (rec->rec_conn->conn_version >= 26) {
		rtrn = cmyth_livetv_chain_select(rec, timeout);
	} else {
		rtrn = cmyth_ringbuf_select(rec, timeout);
	}

	return rtrn;
}

/*
 * cmyth_livetv_get_block(cmyth_recorder_t rec, char *buf, unsigned long len)
 * Scope: PUBLIC
 * Description
 * Read incoming file data off the network into a buffer of length len.
 *
 * Return Value:
 * Sucess: number of bytes read into buf
 * Failure: -1
 */
int
cmyth_livetv_get_block(cmyth_recorder_t rec, char *buf, unsigned long len)
{
	int rtrn;

	if (!rec->rec_connected) {
		return -1;
	}

	if (rec->rec_conn->conn_version >= 26) {
		rtrn = cmyth_livetv_chain_get_block(rec, buf, len);
	} else {
		rtrn = cmyth_ringbuf_get_block(rec, buf, len);
	}

	return rtrn;
}

/*
 * cmyth_livetv_wait()
 *
 * After starting live TV or after a channel change wait here until some
 * recording data is available.
 */
static int
cmyth_livetv_wait(cmyth_recorder_t rec)
{
	int i = 0, rc = -1;
	cmyth_conn_t conn;
	static int failures = 0;

	usleep(250000*failures);

	conn = cmyth_conn_reconnect(rec->rec_conn);

	while (i++ < 10) {
		int len;
		cmyth_proginfo_t prog;
		cmyth_file_t file;

		if (!cmyth_recorder_is_recording(rec)) {
			usleep(1000);
			continue;
		}

		prog = cmyth_recorder_get_cur_proginfo(rec);

		if (prog == NULL) {
			usleep(1000);
			continue;
		}

		file = cmyth_conn_connect_file(prog, conn, 4096, 4096);

		ref_release(prog);

		if (file == NULL) {
			if (failures < 4) {
				failures++;
			}
			usleep(1000);
			continue;
		}

		len = cmyth_file_request_block(file, 512);

		ref_release(file);

		if (len == 512) {
			rc = 0;
			break;
		}

		if (failures < 4) {
			failures++;
		}
		usleep(1000);
	}

	ref_release(conn);

	return rc;
}

int
cmyth_livetv_start(cmyth_recorder_t rec)
{
	int rc = -1;

	if (!rec || !rec->rec_connected) {
		return -1;
	}

	if(rec->rec_conn->conn_version >= 26) {
		if (cmyth_recorder_spawn_chain_livetv(rec) != 0) {
			return -1;
		}

		rc = cmyth_livetv_wait(rec);
	}

	return rc;
}

int
cmyth_livetv_stop(cmyth_recorder_t rec)
{
	if (!rec || !rec->rec_connected) {
		return -1;
	}

	return cmyth_recorder_stop_livetv(rec);
}

int
cmyth_livetv_change_channel(cmyth_recorder_t rec, cmyth_channeldir_t direction)
{
	int rc = -1;

	if (!rec || !rec->rec_connected) {
		return -1;
	}

	if(rec->rec_conn->conn_version >= 26) {
		cmyth_recorder_pause(rec);

		if (cmyth_recorder_change_channel(rec, direction) < 0) {
			return -1;
		}

		rc = cmyth_livetv_wait(rec);

		if (rc == 0) {
			cmyth_chain_switch_last(rec->rec_chain);
		}
	} else {
		/* XXX: ringbuf code? */
	}

	return rc;
}

int
cmyth_livetv_set_channel(cmyth_recorder_t rec, char *name)
{
	int rc = -1;

	if (!rec || !rec->rec_connected) {
		return -1;
	}

	if(rec->rec_conn->conn_version >= 26) {
		cmyth_recorder_pause(rec);

		if (cmyth_recorder_set_channel(rec, name) < 0) {
			return -1;
		}

		rc = cmyth_livetv_wait(rec);

		if (rc == 0) {
			cmyth_chain_switch_last(rec->rec_chain);
		}
	} else {
		/* XXX: ringbuf code? */
	}

	return rc;
}

cmyth_file_t
cmyth_livetv_current_file(cmyth_recorder_t rec)
{
	if (!rec || !rec->rec_connected) {
		return NULL;
	}

	if (rec->rec_conn->conn_version >= 26) {
		return cmyth_chain_current_file(rec->rec_chain);
	} else {
		return cmyth_ringbuf_file(rec);
	}
}
