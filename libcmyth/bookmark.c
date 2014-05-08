/*
 *  Copyright (C) 2005-2014, Jon Gettler
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <cmyth_local.h>


long long cmyth_get_bookmark(cmyth_conn_t conn, cmyth_proginfo_t prog)
{
	unsigned int len = CMYTH_TIMESTAMP_LEN + CMYTH_LONGLONG_LEN + 18;
	char buf[len];
	int err;
	long long ret;
	int count;
	int64_t ll;
	int r;
	char *start_ts_dt;
	start_ts_dt = cmyth_datetime_string(prog->proginfo_rec_start_ts);
	snprintf(buf, sizeof(buf), "%s %ld %s","QUERY_BOOKMARK",
		 prog->proginfo_chanId, start_ts_dt);
	ref_release(start_ts_dt);
	pthread_mutex_lock(&conn->conn_mutex);
	if ((err = cmyth_send_message(conn,buf)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			"%s: cmyth_send_message() failed (%d)\n",
			__FUNCTION__, err);
		ret = err;
		goto out;
	}
	count = cmyth_rcv_length(conn);
	if (count < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			"%s: cmyth_rcv_length() failed (%d)\n",
			__FUNCTION__, count);
		ret = count;
		goto out;
	}
	if ((r=cmyth_rcv_int64(conn, &err, &ll, count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			"%s: cmyth_rcv_int64() failed (%d)\n",
			__FUNCTION__, r);
		ret = err;
		goto out;
	}

	ret = ll;
   out:
	pthread_mutex_unlock(&conn->conn_mutex);
	return ret;
}
	
int cmyth_set_bookmark(cmyth_conn_t conn, cmyth_proginfo_t prog, long long bookmark)
{
	unsigned int len = CMYTH_TIMESTAMP_LEN + CMYTH_LONGLONG_LEN * 2 + 18;
	char buf[len];
	char resultstr[3];
	int r,err;
	int ret;
	int count;
	char *start_ts_dt;
	start_ts_dt = cmyth_datetime_string(prog->proginfo_rec_start_ts);
	if (conn->conn_version >= 66) {
		/*
		 * Since protocol 66 mythbackend expects a single 64 bit integer rather than two 32 bit
		 * hi and lo integers.
		 */
		snprintf(buf, sizeof(buf), "SET_BOOKMARK %ld %s %"PRIu64,
			 prog->proginfo_chanId, start_ts_dt, (int64_t)bookmark);
	}
	else {
		snprintf(buf, sizeof(buf), "SET_BOOKMARK %ld %s %d %d",
			 prog->proginfo_chanId, start_ts_dt,
			 (int32_t)(bookmark >> 32),
			 (int32_t)(bookmark & 0xffffffff));
	}
	ref_release(start_ts_dt);
	pthread_mutex_lock(&conn->conn_mutex);
	if ((err = cmyth_send_message(conn,buf)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			"%s: cmyth_send_message() failed (%d)\n",
			__FUNCTION__, err);
		ret = err;
		goto out;
	}
	count = cmyth_rcv_length(conn);
	if ((r=cmyth_rcv_string(conn,&err,resultstr,sizeof(resultstr),count)) < 0) {
		cmyth_dbg(CMYTH_DBG_ERROR,
			"%s: cmyth_rcv_string() failed (%d)\n",
			__FUNCTION__, err);
		ret = r;
		goto out;
	}
	count -= r;
	if (count == 0) {
		ret = (strncmp(resultstr,"OK",2) == 0);
	} else {
		ret = -1;
		cmyth_dbg(CMYTH_DBG_ERROR, "%s(): %d extra bytes\n",
			  __FUNCTION__, count);
	}
   out:
	pthread_mutex_unlock(&conn->conn_mutex);
	return ret;
}
