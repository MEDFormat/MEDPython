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

    # ----- MED flags test -----
    def test_lh_flags(self):

        ms = MedSession(self.session_path, self.level_2_password)

        # Load LH flags
        orig_lh_flags = ms._get_lh_flags()

        # Change one LH flag
        orig_lh_flags['session_level_lh_flags']['LH_UPDATE_EPHEMERAL_DATA_m12'] = False

        # Set the modified orig flags
        ms._set_lh_flags(orig_lh_flags)

        # Load new LH flags
        new_lh_flags = ms._get_lh_flags()

        # Compare flags
        assert new_lh_flags['session_level_lh_flags']['LH_UPDATE_EPHEMERAL_DATA_m12'] is False

        # Set the flag back
        orig_lh_flags['session_level_lh_flags']['LH_UPDATE_EPHEMERAL_DATA_m12'] = True
        ms._set_lh_flags(orig_lh_flags)

        ms.close()

        return

    def test_dm_flags(self):
        ms = MedSession(self.session_path, self.level_2_password)

        dm = ms.data_matrix

        # Load LH flags
        orig_dm_flags = dm._get_dm_flags()

        # Change one LH flag
        orig_dm_flags['DM_TYPE_SI2_m12'] = True

        # Set the modified orig flags
        dm._set_dm_flags(orig_dm_flags)

        # Load new LH flags
        new_dm_flags = dm._get_dm_flags()

        # Compare flags
        assert new_dm_flags['DM_TYPE_SI2_m12'] is True

        # Set the flag back
        orig_dm_flags['DM_TYPE_SI2_m12'] = False
        dm._set_dm_flags(orig_dm_flags)

        # Test filter_type
        orig_filter_type = dm.filter_type
        dm.filter_type = 'none'
        assert dm.filter_type != orig_filter_type

        # Test detrend
        orig_detrend = dm.detrend
        dm.detrend = not orig_detrend
        assert dm.detrend is not orig_detrend

        # Test trace_ranges
        orig_trace_ranges = dm.trace_ranges
        dm.trace_ranges = not orig_trace_ranges
        assert dm.trace_ranges is not orig_trace_ranges

        # Test major_dimension
        orig_major_dimension = dm.major_dimension
        dm.major_dimension = 'sample'
        assert dm.major_dimension != orig_major_dimension

        ms.close()

    # ----- Read metadata test -----

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
        pass

if __name__ == '__main__':
    unittest.main()
