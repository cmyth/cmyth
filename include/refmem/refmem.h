/*
 *  Copyright (C) 2004-2014, Eric Lund, Jon Gettler
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

/** \file refmem.h
 * A C library for managing reference counted memory allocations.
 */

#ifndef __REFMEM_H
#define __REFMEM_H

#include <unistd.h>

/**
 * Retrieve the major version number of the library.
 * \returns The library major version number.
 */
extern int ref_version_major(void);

/**
 * Retrieve the minor version number of the library.
 * \returns The library minor version number.
 */
extern int ref_version_minor(void);

/**
 * Retrieve the branch version number of the library.
 * \returns The library branch version number.
 */
extern int ref_version_branch(void);

/**
 * Retrieve the fork version number of the library.
 * \returns The library fork version number.
 */
extern int ref_version_fork(void);

/**
 * Retrieve the version number of the library.
 * \returns The library version number string.
 */
extern const char* ref_version(void);

/**
 * Return current number of references outstanding for everything.
 * \returns The number of references (not objects) that currently exist.
 */
extern int ref_get_refcount();

/**
 * Retrieve the number of references and total bytes used.
 */
extern void ref_get_usage(unsigned int *refs, unsigned int *bytes);

/**
 * Release a reference to allocated memory.
 * \param p allocated memory
 */
extern void ref_release(void *p);

/**
 * Add a reference to allocated memory.
 * \param p allocated memory
 * \return new reference
 */
extern void *ref_hold(void *p);

/**
 * Duplicate a string using reference counted memory.
 * \param str string to duplicate
 * \return reference to the duplicated string
 */
extern char *ref_strdup(char *str);

/**
 * Create a reference counted buffer whose contents contain the result of
 * calling sprintf() on the argument list.
 * \param format sprintf() style format list
 * \return reference to the allocated string
 */
extern char *ref_sprintf(const char *format, ...);

/**
 * Allocate reference counted memory. (PRIVATE)
 * \param len allocation size
 * \param file filename
 * \param func function name
 * \param line line number
 * \return reference counted memory
 * \note __ref_alloc() should not be called directly.
 */
extern void *__ref_alloc(size_t len, const char *file, const char *func,
			 int line);

/**
 * Allocate a block of reference counted memory.
 * \param len allocation size
 * \return pointer to reference counted memory block
 */
#if defined(DEBUG)
#define ref_alloc(len) (__ref_alloc((len), __FILE__, __FUNCTION__, __LINE__))
#else
#define ref_alloc(len) (__ref_alloc((len), (char *)0, (char *)0, 0))
#endif

/**
 * Reallocate reference counted memory.
 * \param p allocated memory
 * \param len new allocation size
 * \return reference counted memory
 */
extern void *ref_realloc(void *p, size_t len);

typedef void (*ref_destroy_t)(void *p);

/**
 * Add a destroy callback for reference counted memory.
 * \param block allocated memory
 * \param func destroy function
 */
extern void ref_set_destroy(void *block, ref_destroy_t func);

/**
 * Print allocation information to stdout.
 */
extern void ref_alloc_show(void);

/*
 * Debug level constants used to determine the level of debug tracing
 * to be done and the debug level of any given message.
 */

#define REF_DBG_NONE    -1
#define REF_DBG_ERRORS   0
#define REF_DBG_COUNTERS 1
#define REF_DBG_DEBUG    2
#define REF_DBG_ALL      3

/**
 * Set librefmem debug level.
 * \param l debugging level (-1 for none, 3 for all)
 */
void refmem_dbg_level(int l);

/**
 * Enable all librefmem debugging.
 */
void refmem_dbg_all();

/**
 * Disable all librefmem debugging.
 */
void refmem_dbg_none();

#endif /* __REFMEM_H */
