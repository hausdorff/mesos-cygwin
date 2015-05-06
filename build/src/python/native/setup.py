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

import ext_modules

config = {
    'name': 'mesos.native',
    'version': '0.23.0',
    'description': 'Mesos native driver implementation',
    'author': 'Apache Mesos',
    'author_email': 'dev@mesos.apache.org',
    'url': 'http://pypi.python.org/pypi/mesos.native',
    'namespace_packages': [ 'mesos' ],
    'packages': [ 'mesos', 'mesos.native' ],
    'package_dir': { '': 'src' },
    'install_requires': [ 'mesos.interface == 0.23.0' ],
    'license': 'Apache 2.0',
    'keywords': 'mesos',
    'classifiers': [ ],
    'ext_modules': [ ext_modules.mesos_module ]
}

from setuptools import setup

setup(**config)