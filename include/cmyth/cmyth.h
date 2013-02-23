/*
 *  Copyright (C) 2004-2013, Eric Lund, Jon Gettler
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

#if defined(DOXYGEN_API)
/*! \mainpage cmyth
 *
 * cmyth is a library that provides a C language API to access and control
 * a MythTV backend.  It is released under the LGPL 2.1.
 *
 * This document describes the external API of libcmyth and librefmem.
 *
 * \section projectweb Project website
 * http://cmyth.github.com/
 *
 * \section repos Source repository
 * https://github.com/cmyth/cmyth
 *
 * \section libraries Library APIs
 * \li \link cmyth.h libcmyth \endlink
 * \li \link refmem.h librefmem \endlink
 *
 * \section source Example Source Code
 * \li \link mythcat.c mythcat \endlink
 * \li \link mythfuse.c mythfuse \endlink
 * \li \link mythping.c mythping \endlink
 */
#endif /* DOXYGEN_API */

/** \file cmyth.h
 * A C library for communicating with a MythTV server.
 */

#ifndef __CMYTH_H
#define __CMYTH_H

#include <time.h>
#include <sys/time.h>

#if defined(__GNUC__)
#define CMYTH_DEPRECATED __attribute__ ((__deprecated__))
#else
#define CMYTH_DEPRECATED
#endif

/*
 * -----------------------------------------------------------------
 * Library version information
 * -----------------------------------------------------------------
 */

/**
 * Retrieve the major version number of the library.
 * \returns The library major version number.
 */
extern int cmyth_version_major(void);

/**
 * Retrieve the minor version number of the library.
 * \returns The library minor version number.
 */
extern int cmyth_version_minor(void);

/**
 * Retrieve the branch version number of the library.
 * \returns The library branch version number.
 */
extern int cmyth_version_branch(void);

/**
 * Retrieve the fork version number of the library.
 * \returns The library fork version number.
 */
extern int cmyth_version_fork(void);

/**
 * Retrieve the version number of the library.
 * \returns The library version number string.
 */
extern const char* cmyth_version(void);

/*
 * -----------------------------------------------------------------
 * Opaque structure types
 * -----------------------------------------------------------------
 */

struct cmyth_chain;
struct cmyth_chanlist;
struct cmyth_channel;
struct cmyth_conn;
struct cmyth_commbreak;
struct cmyth_commbreaklist;
struct cmyth_file;
struct cmyth_freespace;
struct cmyth_proginfo;
struct cmyth_proglist;
struct cmyth_recorder;
struct cmyth_timestamp;

/*
 * -----------------------------------------------------------------
 * Opaque structure pointer types
 * -----------------------------------------------------------------
 */

typedef struct cmyth_chain *cmyth_chain_t;

/**
 * \typedef cmyth_chanlist_t
 * A list of recorder channels.
 */
typedef struct cmyth_chanlist *cmyth_chanlist_t;

/**
 * \typedef cmyth_channel_t
 * A channel information structure for a single channel on a recorder.
 */
typedef struct cmyth_channel *cmyth_channel_t;

/**
 * \typedef cmyth_conn_t
 * A connection to a MythTV backend.
 */
typedef struct cmyth_conn *cmyth_conn_t;

/**
 * \typedef cmyth_commbreak_t
 * A description of a single commercial break in a recording.
 */
typedef struct cmyth_commbreak *cmyth_commbreak_t;

/**
 * \typedef cmyth_commbreaklist_t
 * A list of commercial breaks in a recording.
 */
typedef struct cmyth_commbreaklist *cmyth_commbreaklist_t;

/**
 * \typedef cmyth_file_t
 * A connection to a file on a MythTV backend.
 */
typedef struct cmyth_file *cmyth_file_t;

/**
 * \typedef cmyth_freespace_t
 * A structure indicating the amount of total and free space on a MythTV
 * backend.
 */
typedef struct cmyth_freespace *cmyth_freespace_t;

/**
 * \typedef cmyth_proginfo_t
 * The program information structure which describes the recording.
 */
typedef struct cmyth_proginfo *cmyth_proginfo_t;

/**
 * \typedef cmyth_proglist_t
 * A list of program info structures.
 */
typedef struct cmyth_proglist *cmyth_proglist_t;

/**
 * \typedef cmyth_recorder_t
 * A connection to a recorder on a MythTV backend.
 */
typedef struct cmyth_recorder *cmyth_recorder_t;

/**
 * \typedef cmyth_timestamp_t
 * A data structure describing a timestamp with second granularity.
 */
typedef struct cmyth_timestamp *cmyth_timestamp_t;

/*
 * -----------------------------------------------------------------
 * Enums
 * -----------------------------------------------------------------
 */

/**
 * \typedef cmyth_channeldir_t
 * The direction to change a recorder while watching live TV.
 */
typedef enum {
	CHANNEL_DIRECTION_UP = 0,
	CHANNEL_DIRECTION_DOWN = 1,
	CHANNEL_DIRECTION_FAVORITE = 2,
	CHANNEL_DIRECTION_SAME = 4,
} cmyth_channeldir_t;

/**
 * \typedef cmyth_browsedir_t
 * The direction to move when requesting the next program information from
 * a recorders program guide.
 */
typedef enum {
	BROWSE_DIRECTION_SAME = 0,
	BROWSE_DIRECTION_UP = 1,
	BROWSE_DIRECTION_DOWN = 2,
	BROWSE_DIRECTION_LEFT = 3,
	BROWSE_DIRECTION_RIGHT = 4,
	BROWSE_DIRECTION_FAVORITE = 5,
} cmyth_browsedir_t;

/**
 * \typedef cmyth_event_t
 * Events that a MythTV backend can send to a frontend on an event connection.
 */
typedef enum {
	CMYTH_EVENT_UNKNOWN = 0,
	CMYTH_EVENT_CLOSE = 1,
	CMYTH_EVENT_RECORDING_LIST_CHANGE,
	CMYTH_EVENT_RECORDING_LIST_CHANGE_ADD,
	CMYTH_EVENT_RECORDING_LIST_CHANGE_UPDATE,
	CMYTH_EVENT_RECORDING_LIST_CHANGE_DELETE,
	CMYTH_EVENT_SCHEDULE_CHANGE,
	CMYTH_EVENT_DONE_RECORDING,
	CMYTH_EVENT_QUIT_LIVETV,
	CMYTH_EVENT_WATCH_LIVETV,
	CMYTH_EVENT_LIVETV_CHAIN_UPDATE,
	CMYTH_EVENT_SIGNAL,
	CMYTH_EVENT_ASK_RECORDING,
	CMYTH_EVENT_SYSTEM_EVENT,
	CMYTH_EVENT_UPDATE_FILE_SIZE,
	CMYTH_EVENT_GENERATED_PIXMAP,
	CMYTH_EVENT_CLEAR_SETTINGS_CACHE,
	CMYTH_EVENT_ERROR,
	CMYTH_EVENT_COMMFLAG_START,
} cmyth_event_t;

/**
 * \typedef cmyth_proglist_sort_t
 * Different ways that libcmyth can sort a program list.
 */
typedef enum {
	MYTHTV_SORT_DATE_RECORDED = 0,
	MYTHTV_SORT_ORIGINAL_AIRDATE,
} cmyth_proglist_sort_t;

/**
 * \typedef cmyth_proginfo_rec_status_t
 * Program recording status.
 */
typedef enum {
	RS_DELETED = -5,
	RS_STOPPED = -4,
	RS_RECORDED = -3,
	RS_RECORDING = -2,
	RS_WILL_RECORD = -1,
	RS_DONT_RECORD = 1,
	RS_PREVIOUS_RECORDING = 2,
	RS_CURRENT_RECORDING = 3,
	RS_EARLIER_RECORDING = 4,
	RS_TOO_MANY_RECORDINGS = 5,
	RS_CANCELLED = 6,
	RS_CONFLICT = 7,
	RS_LATER_SHOWING = 8,
	RS_REPEAT = 9,
	RS_LOW_DISKSPACE = 11,
	RS_TUNER_BUSY = 12,
} cmyth_proginfo_rec_status_t;

/*
 * -----------------------------------------------------------------
 * Debug Output Control
 * -----------------------------------------------------------------
 */

/*
 * Debug level constants used to determine the level of debug tracing
 * to be done and the debug level of any given message.
 */

#define CMYTH_DBG_NONE  -1
#define CMYTH_DBG_ERROR  0
#define CMYTH_DBG_WARN   1
#define CMYTH_DBG_INFO   2
#define CMYTH_DBG_DETAIL 3
#define CMYTH_DBG_DEBUG  4
#define CMYTH_DBG_PROTO  5
#define CMYTH_DBG_ALL    6

/**
 * Set the libcmyth debug level.
 * \param l level
 */
extern void cmyth_dbg_level(int l);

/**
 * Turn on all libcmyth debugging.
 */
extern void cmyth_dbg_all(void);

/**
 * Turn off all libcmyth debugging.
 */
extern void cmyth_dbg_none(void);

/**
 * Define a callback to use to send messages rather than using stdout
 * \param msgcb function pointer to pass a string to
 */
extern void cmyth_set_dbg_msgcallback(void (*msgcb)(int level,char *));

/*
 * -----------------------------------------------------------------
 * Connection Operations
 * -----------------------------------------------------------------
 */

/**
 * Create a control connection to a backend.
 * \param server server hostname or ip address
 * \param port port number to connect on
 * \param buflen buffer size for the connection to use
 * \param tcp_rcvbuf if non-zero, the TCP receive buffer size for the socket
 * \return control handle
 */
extern cmyth_conn_t cmyth_conn_connect_ctrl(char *server,
					    unsigned short port,
					    unsigned buflen, int tcp_rcvbuf);

/**
 * Create a new control connection based off an existing one.
 * \param conn control handle
 * \return control handle
 */
extern cmyth_conn_t cmyth_conn_reconnect(cmyth_conn_t conn);

/**
 * Create an event connection to a backend.
 * \param server server hostname or ip address
 * \param port port number to connect on
 * \param buflen buffer size for the connection to use
 * \param tcp_rcvbuf if non-zero, the TCP receive buffer size for the socket
 * \return event handle
 */
extern cmyth_conn_t cmyth_conn_connect_event(char *server,
					     unsigned short port,
					     unsigned buflen, int tcp_rcvbuf);

/**
 * Create a file connection to a backend for reading a recording.
 * \param prog program handle
 * \param control control handle
 * \param buflen buffer size for the connection to use
 * \param tcp_rcvbuf if non-zero, the TCP receive buffer size for the socket
 * \return file handle
 */
extern cmyth_file_t cmyth_conn_connect_file(cmyth_proginfo_t prog,
					    cmyth_conn_t control,
					    unsigned buflen, int tcp_rcvbuf);

/**
 * Create a file connection to a backend for reading a recording thumbnail.
 * \param prog program handle
 * \param control control handle
 * \param buflen buffer size for the connection to use
 * \param tcp_rcvbuf if non-zero, the TCP receive buffer size for the socket
 * \return file handle
 */
extern cmyth_file_t cmyth_conn_connect_thumbnail(cmyth_proginfo_t prog,
						 cmyth_conn_t control,
						 unsigned buflen,
						 int tcp_rcvbuf);

/**
 * Create a ring buffer connection to a recorder.
 * \param rec recorder handle
 * \param buflen buffer size for the connection to use
 * \param tcp_rcvbuf if non-zero, the TCP receive buffer size for the socket
 * \retval 0 success
 * \retval -1 error
 */
extern int cmyth_conn_connect_ring(cmyth_recorder_t rec, unsigned buflen,
				   int tcp_rcvbuf);

/**
 * Create a connection to a recorder.
 * \param rec recorder to connect to
 * \param buflen buffer size for the connection to use
 * \param tcp_rcvbuf if non-zero, the TCP receive buffer size for the socket
 * \retval 0 success
 * \retval -1 error
 */
extern int cmyth_conn_connect_recorder(cmyth_recorder_t rec,
				       unsigned buflen, int tcp_rcvbuf);

/**
 * Check whether a block has finished transfering from a backend.
 * \param conn control handle
 * \param size size of block
 * \retval 0 not complete
 * \retval 1 complete
 * \retval <0 error
 */
extern int cmyth_conn_check_block(cmyth_conn_t conn, unsigned long size);

/**
 * Obtain a recorder from a connection by its recorder number.
 * \param conn connection handle
 * \param num recorder number
 * \return recorder handle
 */
extern cmyth_recorder_t cmyth_conn_get_recorder_from_num(cmyth_conn_t conn,
							 int num);

/**
 * Obtain a recorder handle without actually connecting to it.  This can
 * be useful for obtaining information about the recorder or about the
 * programs it will be recording.  The handle cannot be used for a live TV
 * session.
 * \param conn connection handle
 * \param num recorder number
 * \return recorder handle
 */
extern cmyth_recorder_t cmyth_conn_get_recorder(cmyth_conn_t conn, int num);

/**
 * Obtain the next available free recorder on a backend.
 * \param conn connection handle
 * \return recorder handle
 */
extern cmyth_recorder_t cmyth_conn_get_free_recorder(cmyth_conn_t conn);

/**
 * Get the amount of free disk space on a backend.
 * \param control control handle
 * \param[out] total total disk space
 * \param[out] used used disk space
 * \retval 0 success
 * \retval <0 error
 */
extern int cmyth_conn_get_freespace(cmyth_conn_t control,
				    long long *total, long long *used);

/**
 * Determine if a control connection is not responding.
 * \param control control handle
 * \retval 0 not hung
 * \retval 1 hung
 * \retval <0 error
 */
extern int cmyth_conn_hung(cmyth_conn_t control);

/**
 * Determine the number of free recorders.
 * \param conn connection handle
 * \return number of free recorders
 */
extern int cmyth_conn_get_free_recorder_count(cmyth_conn_t conn);

/**
 * Determine the MythTV protocol version being used.
 * \param conn connection handle
 * \return protocol version
 */
extern int cmyth_conn_get_protocol_version(cmyth_conn_t conn);

/**
 * Return a MythTV setting for a hostname
 * \param conn connection handle
 * \param hostname hostname to retreive the setting from
 * \param setting the setting name to get
 * \return ref counted string with the setting
 */
extern char * cmyth_conn_get_setting(cmyth_conn_t conn,
               const char* hostname, const char* setting);

/**
 * Inform the MythTV backend that a shutdown is allowed even though this
 * connction is active.
 * \param conn connection handle
 * \retval 0 success
 * \retval <0 error
 */
extern int cmyth_conn_allow_shutdown(cmyth_conn_t conn);

/**
 * Inform the MythTV backend that a shutdown is not allowed as long as this
 * connction is active.
 * \param conn connection handle
 * \retval 0 success
 * \retval <0 error
 */
extern int cmyth_conn_block_shutdown(cmyth_conn_t conn);

/*
 * -----------------------------------------------------------------
 * Event Operations
 * -----------------------------------------------------------------
 */

/**
 * Retrieve an event from a backend.
 * \param conn connection handle
 * \param[out] data data, if the event returns any
 * \param len size of data buffer
 * \return event type
 */
extern cmyth_event_t cmyth_event_get(cmyth_conn_t conn, char * data, int len);

/**
 * Selects on the event socket, waiting for an event to show up.
 * Allows nonblocking access to events.
 * \param conn connection handle
 * \param timeout an upper bound on the amount of time to wait when non-NULL
 * \retval <0 on failure
 * \retval 0 on timeout
 * \retval >0 when one or more events are available
 */
extern int cmyth_event_select(cmyth_conn_t conn, struct timeval *timeout);

/*
 * -----------------------------------------------------------------
 * Recorder Operations
 * -----------------------------------------------------------------
 */

/**
 * Create a new recorder.
 * \return recorder handle
 */
extern cmyth_recorder_t cmyth_recorder_create(void);

/**
 * Duplicaate a recorder.
 * \param p recorder handle
 * \return duplicated recorder handle
 */
extern cmyth_recorder_t cmyth_recorder_dup(cmyth_recorder_t p);

/**
 * Determine if a recorder is in use.
 * \param rec recorder handle
 * \retval 0 not recording
 * \retval 1 recording
 * \retval <0 error
 */
extern int cmyth_recorder_is_recording(cmyth_recorder_t rec);

/**
 * Determine the framerate for a recorder.
 * \param rec recorder handle
 * \param[out] rate framerate
 * \retval 0 success
 * \retval <0 error
 */
extern int cmyth_recorder_get_framerate(cmyth_recorder_t rec,
					double *rate);

/**
 * Request that the recorder stop transmitting data.
 * \param rec recorder handle
 * \retval 0 success
 * \retval <0 error
 */
extern int cmyth_recorder_pause(cmyth_recorder_t rec);

/**
 * Request that the recorder change the channel being recorded.
 * \param rec recorder handle
 * \param direction direction in which to change channel
 * \retval 0 success
 * \retval <0 error
 */
extern int cmyth_recorder_change_channel(cmyth_recorder_t rec,
					 cmyth_channeldir_t direction);

/**
 * Set the channel for a recorder.
 * \param rec recorder handle
 * \param channame channel name to change to
 * \retval 0 success
 * \retval <0 error
 */
extern int cmyth_recorder_set_channel(cmyth_recorder_t rec,
				      char *channame);

/**
 * Check the validity of a channel for a recorder.
 * \param rec recorder handle
 * \param channame channel name to check
 * \retval <0 not valid
 * \retval 0 valid
 */
extern int cmyth_recorder_check_channel(cmyth_recorder_t rec,
					char *channame);

/**
 * Request the current program info for a recorder.
 * \param rec recorder handle
 * \return program info handle
 */
extern cmyth_proginfo_t cmyth_recorder_get_cur_proginfo(cmyth_recorder_t rec);

/**
 * Request the next program info for a recorder.
 * \param rec recorder handle
 * \param current current program
 * \param direction direction of next program
 * \retval 0 success
 * \retval <0 error
 */
extern cmyth_proginfo_t cmyth_recorder_get_next_proginfo(cmyth_recorder_t rec,
							 cmyth_proginfo_t current,
							 cmyth_browsedir_t direction);

/**
 * Get the filename that the backend is recording live TV into.
 * \param rec recorder handle
 * \return ref counted string containing the filename
 */
extern char* cmyth_recorder_get_filename(cmyth_recorder_t rec);

/**
 * Stop the backend recording of live TV on the specified recorder.
 * \param rec recorder handle
 * \retval 0 success
 * \retval <0 error
 * \note This function seems misnamed.  Should be probably be something
 *       like cmyth_livetv_stop().
 */
extern int cmyth_recorder_stop_livetv(cmyth_recorder_t rec);

/**
 * Retrieve the ID of the specified recorder.
 * \param rec recorder handle
 * \retval >=0 recorder id
 * \retval <0 error
 */
extern int cmyth_recorder_get_recorder_id(cmyth_recorder_t rec);

/**
 * Retrieve the channel list for a specified recorder.
 * \param rec recorder handle
 * \return channel list handle
 */
extern cmyth_chanlist_t cmyth_recorder_get_chanlist(cmyth_recorder_t rec);

/*
 * -----------------------------------------------------------------
 * Channel List Operations
 * -----------------------------------------------------------------
 */

/**
 * Retrieve the number of channels in the list.
 * \param list channel list handle
 * \retval <0 error
 * \retval >=0 number of channels in the list
 */
extern int cmyth_chanlist_get_count(cmyth_chanlist_t list);

/**
 * Retrieve a channel from a channel list.
 * \param list channel list handle
 * \param index list entry to retrieve
 * \returns channel handle
 */
extern cmyth_channel_t cmyth_chanlist_get_item(cmyth_chanlist_t list,
					       unsigned int index);
/*
 * -----------------------------------------------------------------
 * Channel Operations
 * -----------------------------------------------------------------
 */

/**
 * Retrieve the channel ID.
 * \param chan channel handle
 * \returns channel ID
 */
extern long cmyth_channel_id(cmyth_channel_t chan);

/**
 * Retrieve the channel name.
 * \param chan channel handle
 * \returns reference counted channel name
 */
extern char* cmyth_channel_name(cmyth_channel_t chan);

/**
 * Retrieve the channel sign.
 * \param chan channel handle
 * \returns reference counted channel sign
 */
extern char* cmyth_channel_sign(cmyth_channel_t chan);

/**
 * Retrieve the channel string.
 * \param chan channel handle
 * \returns reference counted channel string
 */
extern char* cmyth_channel_string(cmyth_channel_t chan);

/**
 * Retrieve the channel icon.
 * \param chan channel handle
 * \returns reference counted channel icon
 */
extern char* cmyth_channel_icon(cmyth_channel_t chan);

/*
 * -----------------------------------------------------------------
 * Live TV Operations
 * -----------------------------------------------------------------
 */

/**
 * Start recording live TV on a recorder.
 * \param rec recorder handle
 * \retval <0 error
 * \retval 0 success
 */
extern int cmyth_livetv_start(cmyth_recorder_t rec);

/**
 * Stop recording live TV on a recorder.
 * \param rec recorder handle
 * \retval <0 error
 * \retval 0 success
 */
extern int cmyth_livetv_stop(cmyth_recorder_t rec);

/**
 * Block waiting for data on a live TV stream.
 * \param rec recorder handle
 * \param timeout upper bound on the amount of time to block for data
 * \retval <0 error
 * \retval 0 timeout
 * \retval >1 data is available to read
 */
extern int cmyth_livetv_select(cmyth_recorder_t rec, struct timeval *timeout);
  
/**
 * Request data bytes for a live TV stream.
 * \param rec recorder handle
 * \param len number of bytes requested
 * \retval <0 error
 * \retval >=0 number of bytes delivered
 */
extern int cmyth_livetv_request_block(cmyth_recorder_t rec, unsigned long len);

/**
 * Seek to a specified offset in the live TV stream.
 * \param rec recorder handle
 * \param offset offset to seek to
 * \param whence SEEK_SET, SEEK_CUR, or SEEK_END
 * \retval <0 error
 * \retval >=0 new absolute position in the file
 */
extern long long cmyth_livetv_seek(cmyth_recorder_t rec,
				   long long offset, int whence);

/**
 * Get data from a live TV stream.
 * \param rec recorder handle
 * \param buf buffer for retrieving data
 * \param len size of buf
 * \retval <0 error
 * \retval >=0 number of bytes returned in buf
 */
extern int cmyth_livetv_get_block(cmyth_recorder_t rec, char *buf,
                                  unsigned long len);

/**
 * Request that the recorder change the channel being recorded for live TV.
 * \param rec recorder handle
 * \param direction direction in which to change channel
 * \retval 0 success
 * \retval <0 error
 */
extern int cmyth_livetv_change_channel(cmyth_recorder_t rec,
				       cmyth_channeldir_t direction);

/**
 * Request that the recorder change the channel being recorded for live TV.
 * \param rec recorder handle
 * \param direction direction in which to change channel
 * \retval 0 success
 * \retval <0 error
 */
extern int cmyth_livetv_set_channel(cmyth_recorder_t rec, char *name);

/*
 * -----------------------------------------------------------------
 * Live TV Chain Operations
 * -----------------------------------------------------------------
 */

extern cmyth_chain_t cmyth_livetv_get_chain(cmyth_recorder_t rec);

extern int cmyth_chain_set_current(cmyth_chain_t chain, cmyth_proginfo_t prog);

extern int cmyth_chain_switch(cmyth_chain_t chain, int delta);

extern int cmyth_chain_switch_to(cmyth_chain_t chain, int index);

extern int cmyth_chain_switch_last(cmyth_chain_t chain);

extern int cmyth_chain_get_count(cmyth_chain_t chain);

extern cmyth_file_t cmyth_chain_get_file(cmyth_chain_t chain,
					 cmyth_proginfo_t prog);

extern cmyth_proginfo_t cmyth_chain_get_prog(cmyth_chain_t chain,
					     unsigned int which);

extern cmyth_proginfo_t cmyth_chain_get_current(cmyth_chain_t chain);

extern int cmyth_chain_remove_prog(cmyth_chain_t chain, cmyth_proginfo_t prog);

extern int cmyth_chain_set_callback(cmyth_chain_t chain,
				    void (*callback)(cmyth_proginfo_t prog));

/*
 * -----------------------------------------------------------------
 * Timestamp Operations
 * -----------------------------------------------------------------
 */

/**
 * Create a timestamp structure based on an input string in the form
 * of yyyy-mm-ddThh:mm:ss.
 * \param str a timestamp string
 * \retval NULL error
 * \retval non-NULL a timestamp structure describing the timestamp
 */
extern cmyth_timestamp_t cmyth_timestamp_from_string(char *str);

/**
 * Create a timestamp structure based on seconds from the Epoch.
 * \param l seconds since the Epoch
 * \retval NULL error
 * \retval non-NULL a timestamp structure describing the timestamp
 */
extern cmyth_timestamp_t cmyth_timestamp_from_unixtime(time_t l);

/**
 * Return the number of seconds since the Epoch according to the
 * timestamp structure.
 * \param ts timestamp handle
 * \returns seconds since the Epoch
 */
extern time_t cmyth_timestamp_to_unixtime(cmyth_timestamp_t ts);

/**
 * Create a string representing the timestamp.  The resulting format is
 * yyyy-mm-ddThh:mm:ss.
 * \param str character array of at least 20 bytes
 * \param ts timestamp structure
 * \retval <0 error
 * \retval 0 success
 * \note This function should be deprecated since a buffer overflow can
 *       easily occur without specifying the size of str.
 */
extern int cmyth_timestamp_to_string(char *str, cmyth_timestamp_t ts);

/**
 * Create a string representing the timestamp.  The resulting format is
 * yyyy-mm-dd.
 * \param str character array of at least 11 bytes
 * \param ts timestamp structure
 * \retval <0 error
 * \retval 0 success
 * \note This function should be deprecated since a buffer overflow can
 *       easily occur without specifying the size of str.
 */
extern int cmyth_timestamp_to_isostring(char *str, cmyth_timestamp_t ts);

/**
 * Create a string representing the timestamp.  The resulting format is
 * yyyy-mm-ddThh:mm:ss ?M.
 * \param str character array of at least 24 bytes
 * \param ts timestamp structure
 * \param time_format_12 non-zero for 12 hour format
 * \retval <0 error
 * \retval 0 success
 * \note This function should be deprecated since a buffer overflow can
 *       easily occur without specifying the size of str.
 */
extern int cmyth_timestamp_to_display_string(char *str, cmyth_timestamp_t ts,
					     int time_format_12);

/**
 * Create a string representing the timestamp as seconds since the Epoch.
 * \param str character array of at least 11 bytes
 * \param ts timestamp structure
 * \retval <0 error
 * \retval 0 success
 * \note This function should be deprecated since a buffer overflow can
 *       easily occur without specifying the size of str.
 */
extern int cmyth_datetime_to_string(char *str, cmyth_timestamp_t ts);

/**
 * Compare two timestamps.
 * \param ts1 timestamp 1
 * \param ts2 timestamp 2
 * \retval <0 ts1 is earlier than ts2
 * \retval 0 ts1 is equal to ts2
 * \retval >0 ts1 is later than ts2
 */
extern int cmyth_timestamp_compare(cmyth_timestamp_t ts1,
				   cmyth_timestamp_t ts2);

/*
 * -----------------------------------------------------------------
 * Program Info Operations
 * -----------------------------------------------------------------
 */

/**
 * Delete a program.
 * \param control backend control handle
 * \param prog proginfo handle
 * \retval 0 success
 * \retval <0 error
 */
extern int cmyth_proginfo_delete_recording(cmyth_conn_t control,
					   cmyth_proginfo_t prog);

/**
 * Stop a currently recording program.
 * \param control control handle
 * \param prog program handle
 * \retval <0 error
 * \retval 0 success
 */
extern int cmyth_proginfo_stop_recording(cmyth_conn_t control,
					 cmyth_proginfo_t prog);

/**
 * Check a program recording status.
 * \param control control handle
 * \param prog proginfo handle
 * \retval <0 error
 * \retval 0 not recording
 * \retval >0 currently recording on this recorder number
 */
extern int cmyth_proginfo_check_recording(cmyth_conn_t control,
					  cmyth_proginfo_t prog);

/**
 * Delete a program such that it may be recorded again.
 * \param control backend control handle
 * \param prog proginfo handle
 * \retval 0 success
 * \retval <0 error
 */
extern int cmyth_proginfo_forget_recording(cmyth_conn_t control,
					   cmyth_proginfo_t prog);

/**
 * Retrieve the title of a program.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_title(cmyth_proginfo_t prog);

/**
 * Retrieve the subtitle of a program.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_subtitle(cmyth_proginfo_t prog);

/**
 * Retrieve the description of a program.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_description(cmyth_proginfo_t prog);

/**
 * Retrieve the season of a program.
 * \param prog proginfo handle
 * \return season
 */
extern unsigned short cmyth_proginfo_season(cmyth_proginfo_t prog);

/**
 * Retrieve the episode of a program.
 * \param prog proginfo handle
 * \return episode
 */
extern unsigned short cmyth_proginfo_episode(cmyth_proginfo_t prog);

/**
 * Retrieve the category of a program.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_category(cmyth_proginfo_t prog);

/**
 * Retrieve the channel number of a program.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_chanstr(cmyth_proginfo_t prog);

/**
 * Retrieve the channel name of a program.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_chansign(cmyth_proginfo_t prog);

/**
 * Retrieve the channel name of a program.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_channame(cmyth_proginfo_t prog);

/**
 * Retrieve the channel number of a program.
 * \param prog proginfo handle
 * \return channel number
 */
extern long cmyth_proginfo_chan_id(cmyth_proginfo_t prog);

/**
 * Retrieve the pathname of a program file.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_pathname(cmyth_proginfo_t prog);

/**
 * Retrieve the series ID of a program.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_seriesid(cmyth_proginfo_t prog);

/**
 * Retrieve the program ID of a program.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_programid(cmyth_proginfo_t prog);

/**
 * Retrieve the inetref of a program.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_inetref(cmyth_proginfo_t prog);

/**
 * Retrieve the critics rating (number of stars) of a program.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_stars(cmyth_proginfo_t prog);

/**
 * Retrieve the start time of a program.
 * \param prog proginfo handle
 * \return timestamp handle
 */
extern cmyth_timestamp_t cmyth_proginfo_rec_start(cmyth_proginfo_t prog);

/**
 * Retrieve the end time of a program.
 * \param prog proginfo handle
 * \return timestamp handle
 */
extern cmyth_timestamp_t cmyth_proginfo_rec_end(cmyth_proginfo_t prog);

/**
 * Retrieve the original air date of a program.
 * \param prog proginfo handle
 * \return timestamp handle
 */
extern cmyth_timestamp_t cmyth_proginfo_originalairdate(cmyth_proginfo_t prog);

/**
 * Retrieve the recording status of a program.
 * \param prog proginfo handle
 * \return recording status
 */
extern cmyth_proginfo_rec_status_t cmyth_proginfo_rec_status(cmyth_proginfo_t prog);

/**
 * Retrieve the flags associated with a program.
 * \param prog proginfo handle
 * \return flags
 */
extern unsigned long cmyth_proginfo_flags(cmyth_proginfo_t prog);

/**
 * Retrieve the size, in bytes, of a program.
 * \param prog proginfo handle
 * \return program length
 */
extern long long cmyth_proginfo_length(cmyth_proginfo_t prog);

/**
 * Retrieve the hostname of the MythTV backend that recorded a program.
 * \param prog proginfo handle
 * \return MythTV backend hostname
 */
extern char *cmyth_proginfo_host(cmyth_proginfo_t prog);

/**
 * Retrieve the port number for the MythTV backend where a recording
 * is located.
 * \param prog proginfo handle
 * \retval <0 error
 * \retval >=0 port number
 */
extern int cmyth_proginfo_port(cmyth_proginfo_t prog);

/**
 * Determine if two proginfo handles refer to the same program.
 * \param a proginfo handle a
 * \param b proginfo handle b
 * \retval 0 programs are the same
 * \retval -1 programs are different
 */
extern int cmyth_proginfo_compare(cmyth_proginfo_t a, cmyth_proginfo_t b);

/**
 * Retrieve the program length in seconds.
 * \param prog proginfo handle
 * \return program length in seconds
 */
extern int cmyth_proginfo_length_sec(cmyth_proginfo_t prog);

/**
 * Retrieve the start time of a program.
 * \param prog proginfo handle
 * \return timestamp handle
 */
extern cmyth_timestamp_t cmyth_proginfo_start(cmyth_proginfo_t prog);

/**
 * Retrieve the end time of a program.
 * \param prog proginfo handle
 * \return timestamp handle
 */
extern cmyth_timestamp_t cmyth_proginfo_end(cmyth_proginfo_t prog);

/**
 * Retrieve the card ID where the program was recorded.
 * \param prog proginfo handle
 * \return card ID
 */
extern long cmyth_proginfo_card_id(cmyth_proginfo_t prog);

/**
 * Retrieve the recording group of a program.
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_recgroup(cmyth_proginfo_t prog);

/**
 * Retrieve the channel icon path this program info
 * \param prog proginfo handle
 * \return null-terminated string
 */
extern char *cmyth_proginfo_chanicon(cmyth_proginfo_t prog);


/**
 * Retrieve the production year for this program info
 * \param prog proginfo handle
 * \return production year
 */
extern unsigned short cmyth_proginfo_year(cmyth_proginfo_t prog);

/*
 * -----------------------------------------------------------------
 * Program List Operations
 * -----------------------------------------------------------------
 */

/**
 * Retrieve a program list of all recorded programs from the MythTV backend.
 * \param control control handle
 * \retval NULL error
 * \retval non-NULL a program list handle
 */
extern cmyth_proglist_t cmyth_proglist_get_all_recorded(cmyth_conn_t control);

/**
 * Retrieve a program list of all pending recordings from the MythTV backend.
 * \param control control handle
 * \retval NULL error
 * \retval non-NULL a program list handle
 */
extern cmyth_proglist_t cmyth_proglist_get_all_pending(cmyth_conn_t control);

/**
 * Retrieve a program list of all scheduled recordings from the MythTV backend.
 * \param control control handle
 * \retval NULL error
 * \retval non-NULL a program list handle
 */
extern cmyth_proglist_t cmyth_proglist_get_all_scheduled(cmyth_conn_t control);

/**
 * Retrieve a program list of all conflicting recordings from the MythTV
 * backend.
 * \param control control handle
 * \retval NULL error
 * \retval non-NULL a program list handle
 * \note This appears to be broken.  The MythTV protocol command appears to
 *       require a program so it can return a list of programs that conflict
 *       with it.
 */
extern cmyth_proglist_t cmyth_proglist_get_conflicting(cmyth_conn_t control);

/**
 * Retrieve the program info structure at the specified index in the list.
 * \param pl program list handle
 * \param index program index to return
 * \retval NULL error
 * \retval non-NULL program info handle
 */
extern cmyth_proginfo_t cmyth_proglist_get_item(cmyth_proglist_t pl,
						int index);

/**
 * Remove a program from a program list.
 * \param pl program list handle
 * \param prog program info handle
 * \retval <0 error
 * \retval 0 success
 */
extern int cmyth_proglist_delete_item(cmyth_proglist_t pl,
				      cmyth_proginfo_t prog);

/**
 * Retrieve the number of programs in a program list.
 * \param pl program list handle
 * \retval <0 error
 * \retval >=0 number of programs in the program list
 */
extern int cmyth_proglist_get_count(cmyth_proglist_t pl);

/**
 * Sort the program list based on a specified field in the program info.
 * \param pl program list handle
 * \param count number of items to sort
 * \param sort field to sort on
 * \retval <0 error
 * \retval 0 success
 * \note The count paramater should be removed since it seems unlikely that
 *       a user would only want the front of the list sorted.
 */
extern int cmyth_proglist_sort(cmyth_proglist_t pl, int count,
			       cmyth_proglist_sort_t sort);

/*
 * -----------------------------------------------------------------
 * File Transfer Operations
 * -----------------------------------------------------------------
 */

/**
 * Obtain the starting offset in the file of the recording.
 * \param file file handle
 * \note This function is broken when trying to return an error since it
 *       tries to return a negative number.
 */
extern unsigned long long cmyth_file_start(cmyth_file_t file);

/**
 * Obtain the length of a recording in bytes.
 * \param file file handle
 * \note This function is broken when trying to return an error since it
 *       tries to return a negative number.
 */
extern unsigned long long cmyth_file_length(cmyth_file_t file);

/**
 * Read file data requested by cmyth_file_request_block() into the supplied
 * buffer.
 * \param file file handle
 * \param buf data buffer
 * \param len size of buf
 * \retval <0 error
 * \retval >=0 number of bytes read into buf
 */
extern int cmyth_file_get_block(cmyth_file_t file, char *buf,
				unsigned long len);

/**
 * Request some number of bytes from the file be sent from the MythTV backend.
 * The file data can the be read with cmyth_file_get_block().
 * \param file file handle
 * \param len number of bytes to send
 * \retval <0 error
 * \retval >=0 number of bytes sent
 */
extern int cmyth_file_request_block(cmyth_file_t file, unsigned long len);

/**
 * Seek to a specified offset in the file.
 * \param file file handle
 * \param offset offset to seek to
 * \param whence SEEK_SET, SEEK_CUR, or SEEK_END
 * \retval <0 error
 * \retval >=0 new absolute position in the file
 */
extern long long cmyth_file_seek(cmyth_file_t file,
				 long long offset, int whence);

/**
 * Block until file data is available to be read.
 * \param file file handle
 * \param timeout upper limit on the time to block waiting for data
 * \retval <0 error
 * \retval 0 timeout
 * \retval >0 data is available to be read with cmyth_file_get_block()
 */
extern int cmyth_file_select(cmyth_file_t file, struct timeval *timeout);


/*
 * -------
 * Bookmark,Commercial Skip Operations
 * -------
 */

/**
 * Retrieve the bookmark on a recording.
 * \param conn control handle
 * \param prog program info handle
 * \retval <0 error
 * \retval >=0 bookmark offset in frames from the beginning of the program
 */
extern long long cmyth_get_bookmark(cmyth_conn_t conn, cmyth_proginfo_t prog);

/**
 * Set the bookmark on the recording.
 * \param conn control handle
 * \param prog program info handle
 * \param bookmark offset in frames from the beginning of the program.
 * \retval <0 error
 * \retval 0 success
 */
extern int cmyth_set_bookmark(cmyth_conn_t conn, cmyth_proginfo_t prog,
			      long long bookmark);

/**
 * Retrieve the commercial break list for a program.
 * \param conn control handle
 * \param prog program info handle
 * \returns the commercial break list for the program
 */
extern cmyth_commbreaklist_t cmyth_get_commbreaklist(cmyth_conn_t conn, cmyth_proginfo_t prog);

/**
 * Retrieve the cut list for a program.
 * \param conn control handle
 * \param prog program info handle
 * \returns the cut list for the program
 */
extern cmyth_commbreaklist_t cmyth_get_cutlist(cmyth_conn_t conn, cmyth_proginfo_t prog);

/**
 * Retrieve the starting frame mark for the commercial break.
 * \param cb commercial break handle
 * \retval <0 error
 * \retval >=0 starting offset for the commercial break in frames.
 */
extern long long cmyth_commbreak_start_mark(cmyth_commbreak_t cb);

/**
 * Retrieve the ending frame mark for the commercial break.
 * \param cb commercial break handle
 * \retval <0 error
 * \retval >=0 ending offset for the commercial break in frames.
 */
extern long long cmyth_commbreak_end_mark(cmyth_commbreak_t cb);

/**
 * Retrieve the starting byte offset for the commercial break.
 * \param cb commercial break handle
 * \retval <0 error
 * \retval >=0 starting offset for the commercial break in bytes.
 * \note Currently this is only valid if retrieved via MySQL.
 */
extern long long cmyth_commbreak_start_offset(cmyth_commbreak_t cb);

/**
 * Retrieve the ending byte offset for the commercial break.
 * \param cb commercial break handle
 * \retval <0 error
 * \retval >=0 ending offset for the commercial break in bytes.
 * \note Currently this is only valid if retrieved via MySQL.
 */
extern long long cmyth_commbreak_end_offset(cmyth_commbreak_t cb);

/**
 * Retrieve the number of items in the commercial break list.
 * \param cbl commercial break list handle
 * \retval <0 error
 * \retval >=0 number of items in the list
 */
extern int cmyth_commbreak_get_count(cmyth_commbreaklist_t cbl);

/**
 * Retrieve a commercial break item from the list.
 * \param cbl commercial break list handle
 * \param index item index to retrieve
 * \retval NULL error
 * \retval non-NULL ref counted commercial break handle
 */
extern cmyth_commbreak_t cmyth_commbreak_get_item(cmyth_commbreaklist_t cbl,
						  unsigned int index);

/*
 * -----------------------------------------------------------------
 * Optional MySQL Database Operations
 * -----------------------------------------------------------------
 */

#if defined(HAS_MYSQL)

struct cmyth_database;

/**
 * \typedef cmyth_database_t
 * A connection to the MySQL sever on a MythTV backend.
 */
typedef struct cmyth_database *cmyth_database_t;

/**
 * Create a handle (but not a connection) to a MythTV backend MySQL database.
 * \param host hostname
 * \param db_name database name
 * \param user username
 * \param pass password
 * \returns database handle
 */
extern cmyth_database_t cmyth_database_init(char *host, char *db_name,
					    char *user, char *pass);

extern int cmyth_update_bookmark_setting(cmyth_database_t, cmyth_proginfo_t);

extern long long cmyth_get_bookmark_mark(cmyth_database_t, cmyth_proginfo_t, long long, int);
extern int cmyth_get_bookmark_offset(cmyth_database_t db, long chanid, long long mark, char *starttime, int mode);
extern cmyth_commbreaklist_t cmyth_mysql_get_commbreaklist(cmyth_database_t db, cmyth_conn_t conn, cmyth_proginfo_t prog);

/*
 * mysql info
 */

typedef struct cmyth_program {
	int chanid;
	char callsign[30];
	char name[84];
	int sourceid;
	char title[150];
	char subtitle[150];
	char description[280];
	time_t starttime;
	time_t endtime;
	char programid[30];
	char seriesid[24];
	char category[84];
	int recording;
	int rec_status;
	int channum;
	int event_flags;
	int startoffset;
	int endoffset;
}cmyth_program_t;

typedef struct cmyth_recgrougs {
	char recgroups[33];
}cmyth_recgroups_t;

extern int cmyth_mysql_get_recgroups(cmyth_database_t, cmyth_recgroups_t **);
extern int cmyth_mysql_delete_scheduled_recording(cmyth_database_t db, char * query);
extern int cmyth_mysql_insert_into_record(cmyth_database_t db, char * query, char * query1, char * query2, char *title, char * subtitle, char * description, char * callsign);

extern char* cmyth_get_recordid_mysql(cmyth_database_t, int, char *, char *, char *, char *, char *);
extern int cmyth_get_offset_mysql(cmyth_database_t, int, char *, int, char *, char *, char *, char *, char *);

extern int cmyth_mysql_get_prog_finder_char_title(cmyth_database_t db, cmyth_program_t **prog, time_t starttime, char *program_name);
extern int cmyth_mysql_get_prog_finder_time(cmyth_database_t db, cmyth_program_t **prog,  time_t starttime, char *program_name);
extern int cmyth_mysql_get_guide(cmyth_database_t db, cmyth_program_t **prog, time_t starttime, time_t endtime);
extern int cmyth_mysql_testdb_connection(cmyth_database_t db,char **message);
extern int cmyth_schedule_recording(cmyth_conn_t conn, char * msg);
extern char * cmyth_mysql_escape_chars(cmyth_database_t db, char * string);
extern int cmyth_mysql_get_commbreak_list(cmyth_database_t db, int chanid, char * start_ts_dt, cmyth_commbreaklist_t breaklist, int conn_version);

extern int cmyth_mysql_get_prev_recorded(cmyth_database_t db, cmyth_program_t **prog);
extern int cmyth_mythtv_remove_previous_recorded(cmyth_database_t db,char *query);

#endif /* HAS_MYSQL */

/*
 * -----------------------------------------------------------------
 * Externally Deprecated Items (will be hidden inside libcmyth)
 * -----------------------------------------------------------------
 */

/**
 * \defgroup deprecated_external Externally Deprecated Items
 * These items are deprecated from the cmyth API.  They will continue to
 * be used within libcmyth, but they will be removed from the API and
 * hidden within libcmyth in the future.
 * @{
 */

/**
 * \struct cmyth_commbreak
 * Commercial break data structure.
 *
 * \deprecated Use of cmyth_commbreak outside libcmyth is \b deprecated.
 */
struct cmyth_commbreak {
        long long start_mark;
        long long start_offset;
        long long end_mark;
        long long end_offset;
};

/**
 * \struct cmyth_commbreaklist
 * Commercial break list.
 *
 * \deprecated Use of cmyth_commbreaklist outside libcmyth is \b deprecated.
 */
struct cmyth_commbreaklist {
        cmyth_commbreak_t *commbreak_list;
        long commbreak_count;
};

/**
 * \struct cmyth_livetv_chain
 * Live TV chain data structure.
 *
 * \deprecated Use of cmyth_livetv_chain outside libcmyth is \b deprecated.
 * \note This is for MythTV protocol 26 and beyond.
 */
struct cmyth_livetv_chain;

/**
 * \typedef cmyth_livetv_chain_t
 * Live TV chain data structure.
 *
 * \deprecated Use of cmyth_livetv_chain_t outside libcmyth is \b deprecated.
 * \note This is for MythTV protocol 26 and beyond.
 */
typedef struct cmyth_livetv_chain *cmyth_livetv_chain_t;

/**
 * \struct cmyth_ringbuf
 * Live TV ring buffer data structure.
 *
 * \deprecated Use of cmyth_ringbuf outside libcmyth is \b deprecated.
 * \note This is for MythTV protocol 25 and earlier.
 */
struct cmyth_ringbuf;

/**
 * \typedef cmyth_ringbuf_t
 * Live TV ring buffer data structure.
 *
 * \deprecated Use of cmyth_ringbuf_t outside libcmyth is \b deprecated.
 * \note This is for MythTV protocol 25 and earlier.
 */
typedef struct cmyth_ringbuf *cmyth_ringbuf_t;

/**
 * \deprecated Use of cmyth_livetv_chain_create outside libcmyth is
 * \b deprecated.
 * \note This is for MythTV protocol 26 and beyond.
 */
extern cmyth_livetv_chain_t cmyth_livetv_chain_create(char * chainid);

/**
 * \deprecated Use of cmyth_ringbuf_create outside libcmyth is \b deprecated.
 * \note This is for MythTV protocol 25 and earlier.
 */
extern cmyth_ringbuf_t cmyth_ringbuf_create(void);

/**
 * Print a libcmyth debug message.
 * \param level debug level
 * \param fmt printf style format
 * \deprecated Use of cmyth_ringbuf_t outside libcmyth is \b deprecated.
 */
extern void cmyth_dbg(int level, char *fmt, ...);

/**
 * \deprecated Use of cmyth_recorder_spawn_livetv outside libcmyth is
 * \b deprecated.
 * \note This is for MythTV protocol 25 and earlier.
 */
extern int cmyth_recorder_spawn_livetv(cmyth_recorder_t rec);

/**
 * \deprecated Use of cmyth_recorder_done_ringbuf outside libcmyth is
 * \b deprecated.
 * \note This is for MythTV protocol 25 and earlier.
 */
extern int cmyth_recorder_done_ringbuf(cmyth_recorder_t rec);

/**
 * \deprecated Use of cmyth_recorder_spawn_chain_livetv outside libcmyth is
 * \b deprecated.
 * \note This is for MythTV protocol 26 and beyond.
 */
extern int cmyth_recorder_spawn_chain_livetv(cmyth_recorder_t rec);

/**
 * \deprecated Use of cmyth_livetv_spawn_chain_livetv outside libcmyth is
 * \b deprecated.
 * \note This is for MythTV protocol 26 and beyond.
 */
extern int cmyth_livetv_chain_switch(cmyth_recorder_t rec, int dir);

/**
 * \deprecated Use of cmyth_livetv_chain_switch_last outside libcmyth is
 * \b deprecated.
 * \note This is for MythTV protocol 26 and beyond.
 */
extern int cmyth_livetv_chain_switch_last(cmyth_recorder_t rec);

/**
 * \deprecated Use of cmyth_livetv_chain_update outside libcmyth is
 * \b deprecated.
 * \note This is for MythTV protocol 26 and beyond.
 */
extern int cmyth_livetv_chain_update(cmyth_recorder_t rec, char * chainid,
				     int tcp_rcvbuf);

/**
 * \deprecated Use of cmyth_livetv_chain_setup outside libcmyth is
 * \b deprecated.
 * \note This is for MythTV protocol 26 and beyond.
 */
extern cmyth_recorder_t cmyth_livetv_chain_setup(cmyth_recorder_t old_rec,
						 int tcp_rcvbuf,
						 void (*prog_update_callback)(cmyth_proginfo_t));

/**
 * \deprecated Use of cmyth_ringbuf_setup outside libcmyth is
 * \b deprecated.
 * \note This is for MythTV protocol 25 and earlier.
 */
extern cmyth_recorder_t cmyth_ringbuf_setup(cmyth_recorder_t old_rec);

/**
 * \deprecated Use of cmyth_ringbuf_request_block outside libcmyth is
 * \b deprecated.
 * \note This is for MythTV protocol 25 and earlier.
 */
extern int cmyth_ringbuf_request_block(cmyth_recorder_t rec,
				       unsigned long len);

/**
 * \deprecated Use of cmyth_ringbuf_select outside libcmyth is
 * \b deprecated.
 * \note This is for MythTV protocol 25 and earlier.
 */
extern int cmyth_ringbuf_select(cmyth_recorder_t rec, struct timeval *timeout);

/**
 * \deprecated Use of cmyth_ringbuf_get_block outside libcmyth is
 * \b deprecated.
 * \note This is for MythTV protocol 25 and earlier.
 */
extern int cmyth_ringbuf_get_block(cmyth_recorder_t rec,
				   char *buf,
				   unsigned long len);

/**
 * \deprecated Use of cmyth_ringbuf_seek outside libcmyth is
 * \b deprecated.
 * \note This is for MythTV protocol 25 and earlier.
 */
extern long long cmyth_ringbuf_seek(cmyth_recorder_t rec,
				    long long offset,
				    int whence);

/**
 * \deprecated Use of cmyth_ringbuf_read outside libcmyth is
 * \b deprecated.
 * \note This is for MythTV protocol 25 and earlier.
 */
extern int cmyth_ringbuf_read(cmyth_recorder_t rec,
			      char *buf,
			      unsigned long len);

/**
 * \deprecated Use of cmyth_timestamp_create outside libcmyth is
 * \b deprecated.
 */
extern cmyth_timestamp_t cmyth_timestamp_create(void);

/**
 * \deprecated Use of cmyth_proginfo_create outside libcmyth is
 * \b deprecated.
 */
extern cmyth_proginfo_t cmyth_proginfo_create(void);

/**
 * \deprecated Use of cmyth_proglist_create outside libcmyth is
 * \b deprecated.
 */
extern cmyth_proglist_t cmyth_proglist_create(void);

/**
 * \deprecated Use of cmyth_freespace_create outside libcmyth is
 * \b deprecated.
 */
extern cmyth_freespace_t cmyth_freespace_create(void);

/**
 * \deprecated Use of cmyth_commbreaklist_create outside libcmyth is
 * \b deprecated.
 */
extern cmyth_commbreaklist_t cmyth_commbreaklist_create(void);

/**
 * \deprecated Use of cmyth_commbreak_create outside libcmyth is
 * \b deprecated.
 */
extern cmyth_commbreak_t cmyth_commbreak_create(void);

/**
 * \deprecated Use of cmyth_rcv_commbreaklist outside libcmyth is
 * \b deprecated.
 */
extern int cmyth_rcv_commbreaklist(cmyth_conn_t conn, int *err,
				   cmyth_commbreaklist_t breaklist, int count);

/**@}*/

/*
 * -----------------------------------------------------------------
 * Deprecated items (will be removed entirely)
 * -----------------------------------------------------------------
 */

/**
 * \defgroup deprecated_items Deprecated Items
 * These items are deprecated.  They should not be used, and will be
 * removed in the future.
 * @{
 */

/**
 * \deprecated Use of keyframes is \b deprecated.
 */
struct cmyth_keyframe;

/**
 * \deprecated Use of keyframes is \b deprecated.
 */
typedef struct cmyth_keyframe *cmyth_keyframe_t;

/**
 * \deprecated Use of keyframes is \b deprecated.
 */
extern cmyth_keyframe_t cmyth_keyframe_create(void);

/**
 * \deprecated Use of keyframes is \b deprecated.
 */
extern char *cmyth_keyframe_string(cmyth_keyframe_t kf);

/**
 * \deprecated Use of posmaps are \b deprecated.
 */
struct cmyth_posmap;

/**
 * \deprecated Use of posmaps are \b deprecated.
 */
typedef struct cmyth_posmap *cmyth_posmap_t;

/**
 * \deprecated Use of cmyth_rec_num is \b deprecated.
 */
struct cmyth_rec_num;

/**
 * \deprecated Use of cmyth_rec_num_t is \b deprecated.
 */
typedef struct cmyth_rec_num *cmyth_rec_num_t;

/**
 * \deprecated Use of cmyth_tvguide_progs is \b deprecated.
 */
struct cmyth_tvguide_progs;

/**
 * \deprecated Use of cmyth_tvguide_progs_t is \b deprecated.
 */
typedef struct cmyth_tvguide_progs *cmyth_tvguide_progs_t;

/**
 * \deprecated Use of cmyth_rec_num_t is \b deprecated.
 */
extern cmyth_rec_num_t cmyth_rec_num_create(void);

/**
 * \deprecated Use of cmyth_rec_num_t is \b deprecated.
 */
extern cmyth_rec_num_t cmyth_rec_num_get(char *host,
					 unsigned short port,
					 unsigned id);

/**
 * \deprecated Use of cmyth_rec_num_t is \b deprecated.
 */
extern char *cmyth_rec_num_string(cmyth_rec_num_t rn);

/**
 * \deprecated Use of posmaps are \b deprecated.
 */
extern cmyth_posmap_t cmyth_posmap_create(void);

/**
 * \deprecated Use of cmyth_rec_num_t is \b deprecated.
 */
extern int cmyth_proginfo_get_recorder_num(cmyth_conn_t control,
					   cmyth_rec_num_t rnum,
					   cmyth_proginfo_t prog);

#define CMYTH_NUM_SORTS 2

/**
 * \deprecated Use of cmyth_recorder_start_stream is \b deprecated.
 */
extern int cmyth_recorder_start_stream(cmyth_recorder_t rec);

/**
 * \deprecated Use of cmyth_recorder_end_stream is \b deprecated.
 */
extern int cmyth_recorder_end_stream(cmyth_recorder_t rec);

#if defined(HAS_MYSQL)
/**
 * Retrieve the tuner type of a recorder.
 * \deprecated This function seems quite useless.
 */
extern int cmyth_tuner_type_check(cmyth_database_t db, cmyth_recorder_t rec, int check_tuner_enabled);

extern int cmyth_database_set_host(cmyth_database_t db, char *host);
extern int cmyth_database_set_user(cmyth_database_t db, char *user);
extern int cmyth_database_set_pass(cmyth_database_t db, char *pass);
extern int cmyth_database_set_name(cmyth_database_t db, char *name);
#endif /* HAS_MYSQL */

/**
 * \deprecated Use of cmyth_ringbuf_pathname is \b deprecated.
 */
extern char * cmyth_ringbuf_pathname(cmyth_recorder_t rec);

/**
 * \deprecated Use of cmyth_proginfo_get_detail is \b deprecated.
 */
extern cmyth_proginfo_t cmyth_proginfo_get_detail(cmyth_conn_t control,
						  cmyth_proginfo_t p);

/**
 * \deprecated Use of cmyth_file_data is \b deprecated.
 */
extern cmyth_conn_t cmyth_file_data(cmyth_file_t file);

/**
 * \deprecated Use of cmyth_file_set_closed_callback is \b deprecated.
 */
extern void cmyth_file_set_closed_callback(cmyth_file_t file,
					   void (*callback)(cmyth_file_t));

/**
 * \deprecated Use of PROGRAM_ADJUST is \b deprecated.
 */
#define PROGRAM_ADJUST  3600

/**
 * \deprecated Use of cmyth_get_delete_list is \b deprecated.
 */
extern int cmyth_get_delete_list(cmyth_conn_t, char *, cmyth_proglist_t);

/**
 * Start recording live TV on a recorder.
 * \note This function should be replaced with something consistent
 *       with the rest of the API.  Probably cmyth_livetv_start() that takes
 *       a cmyth_conn_t argument without all the other arguments.
 */
extern cmyth_recorder_t cmyth_spawn_live_tv(cmyth_recorder_t rec,
					    unsigned buflen,
					    int tcp_rcvbuf,
					    void (*prog_update_callback)(cmyth_proginfo_t),
					    char ** err);

/**@}*/

/*
 * -----------------------------------------------------------------
 * Reserved items (these may be implemented someday)
 * -----------------------------------------------------------------
 */

/**
 * \defgroup reserved Reserved items
 * These items are currently unimplemented, and simply return errors if
 * they are called.  They should not be used.  In the future, they may
 * be implemented or removed.
 * @{
 */

typedef enum {
	ADJ_DIRECTION_UP = 1,
	ADJ_DIRECTION_DOWN = 0,
} cmyth_adjdir_t;

/**
 * \b Unimplemented.  Do not use.
 * \note This item may either be implemented or removed in the future.
 */
extern int cmyth_recorder_change_color(cmyth_recorder_t rec,
				       cmyth_adjdir_t direction);

/**
 * \b Unimplemented.  Do not use.
 * \note This item may either be implemented or removed in the future.
 */
extern int cmyth_recorder_change_brightness(cmyth_recorder_t rec,
					    cmyth_adjdir_t direction);

/**
 * \b Unimplemented.  Do not use.
 * \note This item may either be implemented or removed in the future.
 */
extern int cmyth_recorder_change_contrast(cmyth_recorder_t rec,
					  cmyth_adjdir_t direction);

/**
 * \b Unimplemented.  Do not use.
 * \note This item may either be implemented or removed in the future.
 */
extern int cmyth_recorder_change_hue(cmyth_recorder_t rec,
				     cmyth_adjdir_t direction);

/**
 * \b Unimplemented.  Do not use.
 * \note This item may either be implemented or removed in the future.
 */
extern long long cmyth_recorder_get_frames_written(cmyth_recorder_t rec);

/**
 * \b Unimplemented.  Do not use.
 * \note This item may either be implemented or removed in the future.
 */
extern long long cmyth_recorder_get_free_space(cmyth_recorder_t rec);

/**
 * \b Unimplemented.  Do not use.
 * \note This item may either be implemented or removed in the future.
 */
extern long long cmyth_recorder_get_keyframe_pos(cmyth_recorder_t rec,
						 unsigned long keynum);

/**
 * \b Unimplemented.  Do not use.
 * \note This item may either be implemented or removed in the future.
 */
extern cmyth_posmap_t cmyth_recorder_get_position_map(cmyth_recorder_t rec,
						      unsigned long start,
						      unsigned long end);

/**
 * \b Unimplemented.  Do not use.
 * \note This item may either be implemented or removed in the future.
 */
extern cmyth_proginfo_t cmyth_recorder_get_recording(cmyth_recorder_t rec);

/**
 * \b Unimplemented.  Do not use.
 * \note This item may either be implemented or removed in the future.
 */
extern int cmyth_recorder_stop_playing(cmyth_recorder_t rec);

/**
 * \b Unimplemented.  Do not use.
 * \note This item may either be implemented or removed in the future.
 */
extern int cmyth_recorder_frontend_ready(cmyth_recorder_t rec);

/**
 * \b Unimplemented.  Do not use.
 * \note This item may either be implemented or removed in the future.
 */
extern int cmyth_recorder_cancel_next_recording(cmyth_recorder_t rec);

/**
 * \b Unimplemented.  Do not use.
 * \note This item may either be implemented or removed in the future.
 */
extern int cmyth_recorder_get_input_name(cmyth_recorder_t rec,
					 char *name,
					 unsigned len);

typedef enum {
	WHENCE_SET = 0,
	WHENCE_CUR = 1,
	WHENCE_END = 2,
} cmyth_whence_t;

/**
 * \b Unimplemented.  Do not use.
 * \note This item may either be implemented or removed in the future.
 */
extern long long cmyth_recorder_seek(cmyth_recorder_t rec,
				     long long pos,
				     cmyth_whence_t whence,
				     long long curpos);

/**
 * \b Unimplemented.  Do not use.
 * \note This item may either be implemented or removed in the future.
 */
extern int cmyth_recorder_finish_recording(cmyth_recorder_t rec);

/**
 * \b Unimplemented.  Do not use.
 * \note This item may either be implemented or removed in the future.
 */
extern int cmyth_recorder_toggle_channel_favorite(cmyth_recorder_t rec);

/**
 * \b Unimplemented.  Do not use.
 * \note This item may either be implemented or removed in the future.
 */
extern int cmyth_recorder_check_channel_prefix(cmyth_recorder_t rec,
					       char *channame);

/**@}*/

#endif /* __CMYTH_H */
