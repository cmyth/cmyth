#!/usr/bin/python
#
# SCons build script for libcmyth
#

import os
import sys
import string

from os import pathsep

def find_binary(filename):
	"""Find a file in the system search path
	"""
	path = os.environ['PATH']
	paths = string.split(path, pathsep)
	for i in paths:
		name = os.path.join(i, filename)
		if os.path.isfile(name):
			return name
	return ''

env = Environment()

if os.environ.has_key('PREFIX'):
    prefix = os.environ['PREFIX']
else:
    prefix = '/usr/local'

env.Replace(PREFIX = prefix)

Export('env')

cmyth = SConscript('libcmyth/SConscript')
refmem = SConscript('librefmem/SConscript')
src = SConscript('src/SConscript')

targets = [ cmyth, refmem, src ]

env.Install(prefix + '/include/cmyth',
            ['include/cmyth/cmyth.h', 'include/cmyth/debug.h'])
env.Install(prefix + '/include/refmem',
            ['include/refmem/refmem.h', 'include/refmem/atomic.h'])

cs = find_binary('cscope')
if cs != '':
	cscope = env.Command([ 'cscope.out', 'cscope.files',
			       'cscope.in.out', 'cscope.po.out' ],
			     [ 'src/mythping' ],
			     [ 'find . -name \*.c -or -name \*.h > cscope.files',
			       '%s -b -q -k' % cs ])
	env.Alias('cscope', [cscope])
	all = targets + [cscope]
else:
	all = targets

env.Alias('install', [prefix])
env.Alias('all', all)

env.Default(targets)

Return('targets')
