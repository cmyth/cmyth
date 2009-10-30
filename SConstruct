#
# SCons build script for libcmyth
# http://www.mvpmc.org/
#

import os
import sys
import string

from os import pathsep

def find_binary(self, filename):
	"""Find a file in the system search path
	"""
	path = os.environ['PATH']
	paths = string.split(path, pathsep)
	for i in paths:
		name = os.path.join(i, filename)
		if os.path.isfile(name):
			return name
	return ''

def cmd_not_found(self, arg):
	"""Abort the build
	"""
	print 'Error: %s not found!' % arg
	env.Exit(1)

#
# Initialize the build environment
#
env = Environment()

env.AddMethod(cmd_not_found, 'cmd_not_found')
env.AddMethod(find_binary, 'find_binary')

if os.environ.has_key('CROSS'):
	cross = os.environ['CROSS']
	env.Replace(CC = cross + 'gcc')
	env.Replace(LD = cross + 'ld')

env.Append(CFLAGS = '-Werror')

#
# Check the command line targets
#
build_cscope = 0
build_doxygen = 0
if 'cscope' in COMMAND_LINE_TARGETS:
	build_cscope = 2
if 'doxygen' in COMMAND_LINE_TARGETS:
	build_doxygen = 2
if 'all' in COMMAND_LINE_TARGETS:
	build_doxygen = 1
	build_cscope = 1

#
# Check for binaries that might be required
#
cs = env.find_binary('cscope')
dox = env.find_binary('doxygen')

#
# Find the install prefix
#
if os.environ.has_key('PREFIX'):
    prefix = os.environ['PREFIX']
else:
    prefix = '/usr/local'

env.Replace(PREFIX = prefix)

Export('env')

#
# source targets
#
cmyth = SConscript('libcmyth/SConscript')
refmem = SConscript('librefmem/SConscript')
src = SConscript('src/SConscript')

targets = [ cmyth, refmem, src ]

#
# install targets
#
env.Install(prefix + '/include/cmyth',
            ['include/cmyth/cmyth.h'])
env.Install(prefix + '/include/refmem',
            ['include/refmem/refmem.h', 'include/refmem/atomic.h'])

all = targets

#
# cscope target
#
if build_cscope > 0 and cs != '':
	cscope = env.Command([ 'cscope.out', 'cscope.files',
			       'cscope.in.out', 'cscope.po.out' ],
			     [ Glob('src/*.[ch]'),
			       Glob('lib*/*.[ch]'),
			       Glob('include/*.h'),
			       Glob('include/*/*.h') ],
			     [ 'find . -name \*.c -or -name \*.h > cscope.files',
			       '%s -b -q -k' % cs ])
	env.Alias('cscope', [cscope])
	all += [cscope]
elif build_cscope > 1:
	env.cmd_not_found('cscope')

#
# doxygen target
#
if build_doxygen > 0 and dox != '':
	doxygen = env.Command([ 'doc' ],
			      [ 'Doxyfile',
				Glob('src/*.[ch]'),
				Glob('lib*/*.[ch]'),
				Glob('include/*.h'),
				Glob('include/*/*.h') ],
                              [ '%s Doxyfile' % dox ])
	env.Alias('doxygen', [doxygen])
	all += [doxygen]
elif build_doxygen > 1:
	env.cmd_not_found('doxygen')

#
# misc build targets
#
env.Alias('install', [prefix])
env.Alias('all', all)
env.Default(targets)

#
# cleanup
#
if 'all' in COMMAND_LINE_TARGETS:
	env.Clean(all, ['.sconf_temp','.sconsign.dblite', 'config.log', 'doc'])
if 'doxygen' in COMMAND_LINE_TARGETS:
	env.Clean(all, ['doc'])
