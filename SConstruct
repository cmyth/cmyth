#
# SCons build script for libcmyth
# http://www.mvpmc.org/
#

import os
import sys
import string

from os import pathsep

def find_binary(self, filename):
    """Find a file in the system search path"""
    path = os.environ['PATH']
    paths = string.split(path, pathsep)
    for i in paths:
        name = os.path.join(i, filename)
        if os.path.isfile(name):
            return name
    return ''

def cmd_not_found(self, arg):
    """Abort the build"""
    print 'Error: %s not found!' % arg
    env.Exit(1)

def shlibsuffix(self, major=-1, minor=-1, branch=-1):
    """Create the proper suffix for the shared library on the current OS."""
    if major == -1:
        if sys.platform == 'darwin':
            return '.dylib'
        else:
            return '.so'
    elif minor == -1:
        if sys.platform == 'darwin':
            return '-%d.dylib' % (major)
        else:
            return '.so.%d' % (major)
    elif branch == -1:
        if sys.platform == 'darwin':
            return '-%d.%d.dylib' % (major, minor)
        else:
            return '.so.%d.%d' % (major, minor)
    else:
        if sys.platform == 'darwin':
            return '-%d.%d.%d.dylib' % (major, minor, branch)
        else:
            return '.so.%d.%d.%d' % (major, minor, branch)

def soname(self, name, major=0, minor=0, branch=0):
    """Create the linker shared object argument for gcc for this OS."""
    if sys.platform == 'darwin':
        return '-Wl,-headerpad_max_install_names,'\
               '-undefined,dynamic_lookup,-compatibility_version,%d.%d.%d,'\
               '-current_version,%d.%d.%d,-install_name,lib%s%s' % \
               (major, minor, branch,
                major, minor, branch,
                name, self.shlibsuffix(major, minor, branch))
    else:
        return '-Wl,-soname,lib%s.so.%d.%d.%d' % (name, major, minor, branch)

#
# Initialize the build environment
#
env = Environment()

env.AddMethod(cmd_not_found, 'cmd_not_found')
env.AddMethod(find_binary, 'find_binary')
env.AddMethod(shlibsuffix, 'shlibsuffix')
env.AddMethod(soname, 'soname')

vars = Variables('cmyth.conf')
vars.Add('CC', '', 'gcc')
vars.Add('LD', '', 'ld')

vars.Update(env)

if 'CC' in os.environ:
    env.Replace(CC = os.environ['CC'])

if 'LD' in os.environ:
    env.Replace(CC = os.environ['LD'])

if 'CROSS' in os.environ:
    cross = os.environ['CROSS']
    env.Append(CROSS = cross)
    env.Replace(CC = cross + 'gcc')
    env.Replace(LD = cross + 'ld')

env.Append(CFLAGS = '-Werror')

#
# SCons builders
#
builder = Builder(action = "ln -s ${SOURCE.file} ${TARGET.file}", chdir = True)
env.Append(BUILDERS = {"Symlink" : builder})

#
# Check the command line targets
#
build_cscope = False
build_doxygen = False
if 'cscope' in COMMAND_LINE_TARGETS:
    build_cscope = True
if 'doxygen' in COMMAND_LINE_TARGETS:
    build_doxygen = True
if 'all' in COMMAND_LINE_TARGETS:
    build_doxygen = True
    build_cscope = True

#
# Check for binaries that might be required
#
cs = env.find_binary('cscope')
dox = env.find_binary('doxygen')

#
# Find the install prefix
#
if 'PREFIX' in os.environ:
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
if build_cscope:
    if cs != '':
        cscope = env.Command([ 'cscope.out', 'cscope.files',
                               'cscope.in.out', 'cscope.po.out' ],
                               [ Glob('src/*.[ch]'),
                                 Glob('lib*/*.[ch]'),
                                 Glob('include/*.h'),
                                 Glob('include/*/*.h') ],
			     [ 'find . -name \*.c -or -name \*.h '\
                               '> cscope.files',
                               '%s -b -q -k' % cs ])
        env.Alias('cscope', [cscope])
        all += [cscope]
    else:
        if not env.GetOption('clean'):
            env.cmd_not_found('cscope')

#
# doxygen target
#
if build_doxygen:
    if dox != '':
        doxygen = env.Command([ 'doc' ],
                              [ 'Doxyfile',
                                Glob('src/*.[ch]'),
                            Glob('lib*/*.[ch]'),
                            Glob('include/*.h'),
                            Glob('include/*/*.h') ],
            [ '%s Doxyfile' % dox ])
        env.Alias('doxygen', [doxygen])
        all += [doxygen]
    else:
        if not env.GetOption('clean'):
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
    env.Clean(all, ['doc', 'cmyth.conf'])
    env.Clean(all, [ 'config.log','.sconf_temp','.sconsign.dblite',
                     'xcode/build' ])
if 'doxygen' in COMMAND_LINE_TARGETS:
    env.Clean(all, ['doc'])

if not env.GetOption('clean'):
    vars.Save('cmyth.conf', env)

