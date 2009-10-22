#!/usr/bin/python
#
# SCons build script for libcmyth
#

import os

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

env.Alias('install', [prefix])

env.Install(prefix + '/include/cmyth',
            ['include/cmyth/cmyth.h', 'include/cmyth/debug.h'])
env.Install(prefix + '/include/refmem',
            ['include/refmem/refmem.h', 'include/refmem/atomic.h'])

Return('targets')
