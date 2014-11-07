#!/bin/sh
#
# Do a simple sanity test of a build without needing to install it.
#

TOP=`git rev-parse --show-toplevel`

if [ "${TOP}" = "--show-toplevel" ] ; then
    TOP=`pwd`
fi

TESTDIR=${TOP}

LIBCMYTH=${TOP}/libcmyth
LIBREFMEM=${TOP}/librefmem

LIBRARY_PATH=${LIBCMYTH}:${LIBREFMEM}

export LD_LIBRARY_PATH=${LIBRARY_PATH}
export DYLD_LIBRARY_PATH=${LIBRARY_PATH}

${TESTDIR}/src/mythping $@
