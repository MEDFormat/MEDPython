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

import json
from setuptools import Extension, setup
import numpy

from setuptools.command.install import install

class CustomInstallCommand(install):
    def run(self):

        with open('dhn_medlib/medlib_m12.h') as f:
            lines = f.readlines()

        flag_dict = {
            'LH': {},
            'DM': {}
        }

        for line in lines:
            if line.startswith('#define'):
                mod_line = line.replace('#define ', '').replace('\n', '')
                mod_line = mod_line.lstrip()

                # Remove commens
                mod_line = mod_line.split('//')[0]

                for prefix in ['LH', 'DM']:
                    if mod_line.startswith(prefix) and 'ui8' in mod_line and '<<' in mod_line:
                        def_val = [x for x in mod_line.split('\t') if len(x)]
                        val_str = def_val[1]
                        pos_str = val_str.split('<<')[1]
                        pos = int(pos_str.replace(')', '').strip())

                        flag_dict[prefix][def_val[0]] = pos

        json_string = json.dumps(flag_dict)

        with open("src/dhn_med_py/medlib_flags.py", "w") as outfile:
            outfile.write(f'FLAGS = {json_string}')
        install.run(self)

FILE_EXT = Extension("dhn_med_py.med_file.dhnmed_file",
             ["src/dhn_med_py/med_file/dhnmed_file.c"],
                     include_dirs=["dhn_medlib"],
                     extra_compile_args=['-fms-extensions', '-w', '-O0'])

setup(name="dhn_med_py",
      zip_safe=False,
      package_dir={"": "src"},
      cmdclass={'install': CustomInstallCommand},
      ext_modules=[FILE_EXT],
      include_dirs=[numpy.get_include()])
