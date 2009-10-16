#!/usr/bin/python
#
# SCons build script for libcmyth
#

import os

env = Environment (ENV = os.environ)

Export('env')

cmyth = SConscript('libcmyth/SConscript')

refmem = SConscript('librefmem/SConscript')

libs = [ cmyth, refmem ]

Return('libs')
