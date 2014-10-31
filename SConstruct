#
# SCons build script for libcmyth
# http://cmyth.github.com/
#

import os
import sys
import string

from os import pathsep

import SCons.Builder

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

def shlibsuffix(self, major=-1, minor=-1, branch=-1, fork=-1):
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
    elif fork == -1 or fork == 0:
        if sys.platform == 'darwin':
            return '-%d.%d.%d.dylib' % (major, minor, branch)
        else:
            return '.so.%d.%d.%d' % (major, minor, branch)
    else:
        if sys.platform == 'darwin':
            return '-%d.%d.%d.%d.dylib' % (major, minor, branch, fork)
        else:
            return '.so.%d.%d.%d.%d' % (major, minor, branch, fork)

def soname(self, name, major=0, minor=0, branch=0, fork=0):
    """Create the linker shared object argument for gcc for this OS."""
    if fork > 0:
        version = '%d.%d.%d.%d' % (major, minor, branch, fork)
    else:
        version = '%d.%d.%d' % (major, minor, branch)
    if sys.platform == 'darwin':
        return '-Wl,-headerpad_max_install_names,'\
               '-undefined,dynamic_lookup,-compatibility_version,%s,'\
               '-current_version,%s,-install_name,lib%s%s' % \
               (version, version, name,
                self.shlibsuffix(major, minor, branch, fork))
    else:
        return '-Wl,-soname,lib%s.so.%s' % (name, version)

def build_shared(self):
    """Determine if shared objects should be built for this OS."""
    if sys.platform == 'cygwin':
        return False
    else:
        return True

def shared_library(env, target, source, **kw):
    """Create a shared library and the symlinks pointing to it."""
    version = kw['VERSION']
    major = version[0]
    minor = version[1]
    branch = version[2]
    fork = version[3]
    kw['SHLIBSUFFIX'] = env.shlibsuffix(major, minor, branch, fork)
    shared = env.SharedLibrary(target, source, **kw)
    if fork == 0:
        link0 = shared
    else:
        link0 = env.Symlink('lib%s%s' % (target, env.shlibsuffix(major, minor, branch)), shared)
    link1 = env.Symlink('lib%s%s' % (target, env.shlibsuffix(major, minor)), link0)
    link2 = env.Symlink('lib%s%s' % (target, env.shlibsuffix(major)), link1)
    link3 = env.Symlink('lib%s%s' % (target, env.shlibsuffix()), link2)
    return [ shared, link0, link1, link2, link3 ]

def install_shared(env, target, source, **kw):
    """Install a shared library and the symlinks pointing to it."""
    name = kw['NAME']
    version = kw['VERSION']
    major = version[0]
    minor = version[1]
    branch = version[2]
    fork = version[3]
    lib = env.Install(prefix + '/lib', source[0])
    if fork == 0:
        lib0 = lib
    else:
        lib0 = env.Symlink('%s/lib/lib%s%s' % (prefix, name,
                                               env.shlibsuffix(major, minor, branch)), lib0)
    lib1 = env.Symlink('%s/lib/lib%s%s' % (prefix, name,
                                           env.shlibsuffix(major, minor)), lib0)
    lib2 = env.Symlink('%s/lib/lib%s%s' % (prefix, name,
                                           env.shlibsuffix(major)), lib1)
    lib3 = env.Symlink('%s/lib/lib%s%s' % (prefix, name,
                                           env.shlibsuffix()), lib2)
    return [ lib, lib0, lib1, lib2, lib3 ]

def gen_version(env, target, source=None, **kw):
    version = kw['VERSION']
    major = version[0]
    minor = version[1]
    branch = version[2]
    fork = version[3]
    path = os.path.dirname(Dir(target[0]).abspath) + '/' + target
    f = open(path, 'w')
    f.write('#define VERSION_MAJOR %d\n' % major)
    f.write('#define VERSION_MINOR %d\n' % minor)
    f.write('#define VERSION_BRANCH %d\n' % branch)
    f.write('#define VERSION_FORK %d\n' % fork)
    f.close()
    return [ path ]

#
# Initialize the build environment
#
env = Environment()

env.AddMethod(cmd_not_found, 'cmd_not_found')
env.AddMethod(find_binary, 'find_binary')
env.AddMethod(shlibsuffix, 'shlibsuffix')
env.AddMethod(soname, 'soname')
env.AddMethod(build_shared, 'build_shared')
env.AddMethod(shared_library, 'CMSharedLibrary')
env.AddMethod(install_shared, 'InstallShared')
env.AddMethod(gen_version, 'GenVersion')

cflags = '-Wall -Wextra -Werror -Wno-unused-parameter'
ldflags = ''

vars = Variables('cmyth.conf')
vars.Add('CC', '', 'cc')
vars.Add('LD', '', 'ld')
vars.Add('CFLAGS', '', cflags)
vars.Add('LDFLAGS', '', ldflags)
vars.Add('HAS_MYSQL', '', '')

vars.Update(env)

if 'CC' in os.environ:
    env.Replace(CC = os.environ['CC'])

if 'LD' in os.environ:
    env.Replace(LD = os.environ['LD'])

if 'CFLAGS' in os.environ:
    cflags = os.environ['CFLAGS']
    env.Replace(CFLAGS = cflags)

if 'LDFLAGS' in os.environ:
    env.Replace(LDFLAGS = os.environ['LDFLAGS'])

if 'CROSS' in os.environ:
    cross = os.environ['CROSS']
    env.Append(CROSS = cross)
    env.Replace(CC = cross + 'gcc')
    env.Replace(LD = cross + 'ld')

if 'DEBUG' in os.environ:
    env.Replace(CFLAGS = cflags + ' -g -DDEBUG')

if 'NO_MYSQL' in os.environ:
    env.Replace(HAS_MYSQL = 'no')
elif env['HAS_MYSQL'] == '':
    conf = Configure(env)
    if conf.CheckCHeader('mysql/mysql.h') and conf.CheckLib('mysqlclient'):
        conf.env.Replace(HAS_MYSQL = 'yes')
    env = conf.Finish()

if env['HAS_MYSQL'] == 'yes':
    env.Append(CPPFLAGS = '-DHAS_MYSQL')

conf = Configure(env)
if conf.CheckLib('c', 'arc4random_uniform', autoadd=0):
    env.Append(CPPFLAGS = '-DHAS_ARC4RANDOM')
env = conf.Finish()

#
# SCons builders
#
def create_link(target, source, env):
    os.symlink(os.path.basename(str(source[0])),
               os.path.abspath(str(target[0])))

builder = SCons.Builder.Builder(action = create_link)
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
    if not prefix[0] == '/':
        prefix = os.getcwd() + '/' + prefix
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

env.Depends(cmyth, refmem)

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
        internal = env.Command([ 'doc/all/html/index.html' ],
                               [ 'doc/Doxyfile.all',
                                 Glob('src/*.[ch]'),
                                 Glob('lib*/*.[ch]'),
                                 Glob('include/*.h'),
                                 Glob('include/*/*.h') ],
                               [ '%s doc/Doxyfile.all' % dox ])
        external = env.Command([ 'doc/api/html/index.html' ],
                               [ 'doc/Doxyfile.api',
                                 Glob('src/*.[ch]'),
                                 Glob('include/*/*.h') ],
                               [ '%s doc/Doxyfile.api' % dox ])
        manpages = env.Command([ 'doc/man/man3/cmyth.h.3' ],
                               [ 'doc/Doxyfile.man',
                                 Glob('include/*/*.h') ],
                               [ '%s doc/Doxyfile.man' % dox ])
        doxygen = [ internal, external, manpages ]
        env.Alias('doxygen', doxygen)
        all += doxygen
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
    env.Clean(all, ['doc/all', 'doc/api', 'doc/man', 'cmyth.conf'])
    env.Clean(all, [ 'config.log','.sconf_temp','.sconsign.dblite',
                     'xcode/build' ])
if 'doxygen' in COMMAND_LINE_TARGETS:
    env.Clean(all, ['doc/all', 'doc/api', 'doc/man'])

if not env.GetOption('clean'):
    vars.Save('cmyth.conf', env)

env.Clean('distclean', [ '.sconsign.dblite', '.sconf_temp', 'config.log',
                         'cmyth.conf' ])
