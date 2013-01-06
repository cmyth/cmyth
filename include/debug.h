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

/** \file debug.h
 * A C library for generating and controlling debug output
 */

#ifndef __CMYTH_DEBUG_H
#define  __CMYTH_DEBUG_H

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#if defined ANDROID
#include <android/log.h>
#endif

typedef struct {
	char *name;
	int  cur_level;
	int  (*selector)(int plevel, int slevel);
	void (*msg_callback)(int level, char *msg);
} cmyth_debug_ctx_t;

/**
 * Make a static initializer for an cmyth_debug_ctx_t which provides a debug
 * context for a subsystem.  This is a macro.
 * \param n subsystem name (a static string is fine)
 * \param l initial debug level for the subsystem
 * \param s custom selector function pointer (NULL is okay)
 */
#define CMYTH_DEBUG_CTX_INIT(n,l,s) { n, l, s, NULL }

/**
 * Set the debug level to be used for the subsystem
 * \param ctx the subsystem debug context to use
 * \param level the debug level for the subsystem
 * \return an integer subsystem id used for future interaction
 */
static inline void
__cmyth_dbg_setlevel(cmyth_debug_ctx_t *ctx, int level)
{
	if (ctx != NULL) {
		ctx->cur_level = level;
	}
}

/**
 * Generate a debug message at a given debug level
 * \param ctx the subsystem debug context to use
 * \param level the debug level of the debug message
 * \param fmt a printf style format string for the message
 * \param ap arguments to the format
 */
static inline void
__cmyth_dbg(cmyth_debug_ctx_t *ctx, int level, char *fmt, va_list ap)
{
	char msg[4096];
	int len;
	if (!ctx) {
		return;
	}
	if ((ctx->selector && ctx->selector(level, ctx->cur_level)) ||
	    (!ctx->selector && (level < ctx->cur_level))) {
		len = snprintf(msg, sizeof(msg), "(%s)", ctx->name);
		vsnprintf(msg + len, sizeof(msg)-len, fmt, ap);
		if (ctx->msg_callback) {
			ctx->msg_callback(level, msg);
		} else {
#if defined ANDROID
			__android_log_print(ANDROID_LOG_INFO, "cmyth_dbg", buf);
#else
			fwrite(msg, strlen(msg), 1, stdout);
#endif
		}
	}
}

#endif /*  __CMYTH_DEBUG_H */
