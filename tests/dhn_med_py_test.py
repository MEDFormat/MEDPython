#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Thu Jun 29 09:23:17 2017

Unit testing for pymef library.

Ing.,Mgr. (MSc.) Jan Cimbálník
Biomedical engineering
International Clinical Research Center
St. Anne's University Hospital in Brno
Czech Republic
&
Mayo systems electrophysiology lab
Mayo Clinic
200 1st St SW
Rochester, MN
United States
"""

# Standard library imports
import unittest
import warnings

# Third party imports
import numpy as np

# Local imports
from dhn_med_py.med_session import MedSession


class DhnMedPyTest(unittest.TestCase):

    @classmethod
    def setUpClass(self):

        self.level_1_password = 'L1_password'
        self.level_2_password = 'L2_password'
        self.session_path = 'var_sf.medd'

        self.session = MedSession(self.session_path, self.level_2_password)


    # ----- Read metadata tests -----

    def test_read_ts_segment_metadata(self):
        pass

    # ----- Pymef helpers -----

    def test_wrong_password(self):
        pass

    def test_level_1_password(self):
        pass

    def test_level_2_password(self):
        pass

    def tearDown(self):
        self.session.close()

if __name__ == '__main__':
    unittest.main()
