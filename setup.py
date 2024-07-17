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


from setuptools import Extension, setup
import numpy

from pathlib import Path

this_directory = Path(__file__).parent
long_description = (this_directory / "README.md").read_text()

# the c extension module
DHNMED_FILE_EXT = Extension("dhn_med_py.med_file.dhnmed_file",
                            ["dhn_med_py/med_file/dhnmed_file.c"],
                            include_dirs=["dhn_medlib"],
                            # extra_compile_args=['-w'])
                            # extra_compile_args=['-O3'])
                            extra_compile_args=['-fms-extensions'])

setup(
    name="dhn_med_py",
    version='1.1.2',
    description="Python wrapper for MED format",
    author="Dark Horse Neuro, Inc.",
    download_url="https://medformat.org",
    platforms=["any"],
    url="https://medformat.org",
    license='GPLv3',
    project_urls={"Source": "https://medformat.org"},
    packages=["dhn_med_py", "dhn_med_py.med_file"],
    ext_modules=[DHNMED_FILE_EXT],
    setup_requires=["setuptools_scm"],
    install_requires=['numpy'],
    include_dirs=[numpy.get_include()],
    long_description=long_description,
    long_description_content_type='text/markdown',
    license_files=('LICENSE.txt',),
    classifiers=[
        'License :: OSI Approved :: GNU General Public License v3 (GPLv3)',
        'Operating System :: MacOS :: MacOS X',
        'Operating System :: POSIX :: Linux',
        'Operating System :: Microsoft :: Windows',
        'Programming Language :: Python :: 3',
        'Development Status :: 5 - Production/Stable',
        'Topic :: Scientific/Engineering',
    ],
)
