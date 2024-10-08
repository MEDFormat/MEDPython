# This file is part of the DHN-MED-Py distribution.
# Copyright (c) 2023 Dark Horse Neuro Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

# ***********************************************************************//
# ******************  DARK HORSE NEURO MED Python API  ******************//
# ***********************************************************************//

# Written by Matt Stead and Dan Crepeau
# Copyright Dark Horse Neuro Inc, 2023

# !/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
from setuptools import Extension, setup
import numpy

# Detect operating system
os = sys.platform
if os == 'linux':
    target = "LINUX_m12"
elif os == 'darwin':
    target = "MACOS_m12"
elif os == 'win32':
    target = "WINDOWS_m12"
else:
    target = "UNKNOWN"

FILE_EXT = Extension("dhn_med_py.med_file.dhnmed_file",
             ["src/dhn_med_py/med_file/dhnmed_file.c"],
                     include_dirs=["dhn_medlib"],
                     extra_compile_args=['-fms-extensions',
                                         '-D'+target,
                                         '-w', '-O0'])

setup(name="dhn_med_py",
      zip_safe=False,
      package_dir={"": "src"},
      ext_modules=[FILE_EXT],
      include_dirs=[numpy.get_include()])
