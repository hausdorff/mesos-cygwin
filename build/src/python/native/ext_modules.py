# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import errno
import glob
import os
import shutil

from setuptools import Extension

abs_top_srcdir = '/home/aclemmer/src/mesos/build/..'
abs_top_builddir = '/home/aclemmer/src/mesos/build'

src_python_native = os.path.join(
    'src', 'python', 'native', 'src', 'mesos', 'native')

leveldb = os.path.join('3rdparty', 'leveldb')
zookeeper = os.path.join('3rdparty', 'zookeeper-3.4.5', 'src', 'c')
libprocess = os.path.join('3rdparty', 'libprocess')

# Even though a statically compiled libprocess should include glog,
# libev, gperftools, and protobuf before installation this isn't the
# case, so while a libtool managed build will correctly pull in these
# libraries when building the final result, we need to explicitly
# include them here (or more precisely, down where we actually include
# libev.a and libprofiler.a).
glog = os.path.join(libprocess, '3rdparty', 'glog-0.3.3')
libev = os.path.join(libprocess, '3rdparty', 'libev-4.15')
gperftools = os.path.join(libprocess, '3rdparty', 'gperftools-2.0')
protobuf = os.path.join(libprocess, '3rdparty', 'protobuf-2.5.0')

# Build the list of source files. Note that each source must be
# relative to our current directory (where this script lives).
SOURCES = [
    os.path.join('src', 'mesos', 'native', file)
        for file in os.listdir(os.path.join(abs_top_srcdir, src_python_native))
            if file.endswith('.cpp')
]

INCLUDE_DIRS = [
    os.path.join(abs_top_srcdir, 'include'),
    os.path.join(abs_top_builddir, 'include'),
    # Needed for the *.pb.h protobuf includes.
    os.path.join(abs_top_builddir, 'include', 'mesos'),
    os.path.join(abs_top_builddir, 'src'),
    os.path.join(abs_top_builddir, src_python_native),
    os.path.join(abs_top_builddir, protobuf, 'src'),
]

LIBRARY_DIRS = []

EXTRA_OBJECTS = [
    os.path.join(abs_top_builddir, 'src', '.libs', 'libmesos_no_3rdparty.a'),
    os.path.join(abs_top_builddir, libprocess, '.libs', 'libprocess.a')
]

# For leveldb, we need to check for the presence of libleveldb.a, since
# it is possible to disable leveldb inside mesos.
libev = os.path.join(abs_top_builddir, libev, '.libs', 'libev.a')
libglog = os.path.join(abs_top_builddir, glog, '.libs', 'libglog.a')
libleveldb = os.path.join(abs_top_builddir, leveldb, 'libleveldb.a')
libzookeeper = os.path.join(
    abs_top_builddir, zookeeper, '.libs', 'libzookeeper_mt.a')
libprotobuf = os.path.join(
    abs_top_builddir, protobuf, 'src', '.libs', 'libprotobuf.a')

if os.path.exists(libleveldb):
    EXTRA_OBJECTS.append(libleveldb)
else:
    EXTRA_OBJECTS.append('-lleveldb')

if os.path.exists(libzookeeper):
    EXTRA_OBJECTS.append(libzookeeper)
else:
    EXTRA_OBJECTS.append('-lzookeeper_mt')

if os.path.exists(libev):
    EXTRA_OBJECTS.append(libev)
else:
    EXTRA_OBJECTS.append('-lev')

if os.path.exists(libglog):
    EXTRA_OBJECTS.append(libglog)
else:
    EXTRA_OBJECTS.append('-lglog')

if os.path.exists(libprotobuf):
  EXTRA_OBJECTS.append(libprotobuf)
else:
  EXTRA_OBJECTS.append('-lprotobuf')

# For gperftools, we need to check for the presence of libprofiler.a, since
# it is possible to disable perftools inside libprocess.
libprofiler = os.path.join(
    abs_top_builddir, gperftools, '.libs', 'libprofiler.a')

if os.path.exists(libprofiler):
    EXTRA_OBJECTS.append(libprofiler)

EXTRA_LINK_ARGS = []

# Add any flags from LDFLAGS.
if 'LDFLAGS' in os.environ:
    for flag in os.environ['LDFLAGS'].split():
        EXTRA_LINK_ARGS.append(flag)

# Add any libraries from LIBS.
if 'LIBS' in os.environ:
    for library in os.environ['LIBS'].split():
        EXTRA_LINK_ARGS.append(library)

DEPENDS = [
    os.path.join(abs_top_srcdir, 'src', 'python', source)
        for source in SOURCES
]

# Note that we add EXTRA_OBJECTS to our dependency list to make sure
# that we rebuild this module when one of them changes (e.g.,
# libprocess).
mesos_module = \
    Extension('mesos.native._mesos',
              sources = SOURCES,
              include_dirs = INCLUDE_DIRS,
              library_dirs = LIBRARY_DIRS,
              extra_objects = EXTRA_OBJECTS,
              extra_link_args = EXTRA_LINK_ARGS,
              depends = EXTRA_OBJECTS,
              language = 'c++',
              )
