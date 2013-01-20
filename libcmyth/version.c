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

#include <cmyth/cmyth.h>
#include <refmem/refmem.h>

#include "version.h"

#define xstr(x) str(x)
#define str(x) #x

int
cmyth_version_major(void)
{
	return VERSION_MAJOR;
}

int
cmyth_version_minor(void)
{
	return VERSION_MINOR;
}

int
cmyth_version_branch(void)
{
	return VERSION_BRANCH;
}

int
cmyth_version_fork(void)
{
	return VERSION_FORK;
}

const char*
cmyth_version(void)
{
	return xstr(VERSION_MAJOR) "." xstr(VERSION_MINOR) "." xstr(VERSION_BRANCH) "." xstr(VERSION_FORK);
}
