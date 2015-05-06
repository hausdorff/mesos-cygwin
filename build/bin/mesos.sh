#!/bin/sh

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

# This is a wrapper for running mesos-tests from the build directory
# that first sets up environment variables as appropriate.

# Add any scripts from 'src/cli'.
PATH=/home/aclemmer/src/mesos/build/../src/cli:${PATH}

# Add the executables (or rather, their libtool wrappers) from 'src'.
PATH=/home/aclemmer/src/mesos/build/src:${PATH}

export PATH

# Add 'src/cli/python' to PYTHONPATH.
# TODO(benh): Remove this if/when we install the 'mesos' module via
# PIP and setuptools.
PYTHONPATH=/home/aclemmer/src/mesos/build/../src/cli/python:${PYTHONPATH}

export PYTHONPATH

exec /home/aclemmer/src/mesos/build/src/mesos "${@}"