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

        # self.mef_session = MedSession(self.session_path, self.level_2_password)

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


    def test_set_channel(self):
        ms = MedSession(self.session_path, self.level_2_password)

        channel_names = ms.get_channel_names()

        ms.set_channel_active(channel_names, False)
        ms.set_channel_active(channel_names[0], True)

        lh_flags = ms._get_lh_flags()

        for channel in channel_names:

            if channel == channel_names[0]:
                assert lh_flags['channels'][channel]['channel_level_lh_flags']['LH_CHANNEL_ACTIVE_m12'] is True
            else:
                assert lh_flags['channels'][channel]['channel_level_lh_flags']['LH_CHANNEL_ACTIVE_m12'] is False

        ms.close()

    def test_set_reference_channel(self):
        ms = MedSession(self.session_path, self.level_2_password)

        ref_channel = ms.reference_channel

        channel_names = ms.get_channel_names()

        channel_names.pop(channel_names.index(ref_channel))

        ms.reference_channel = channel_names[0]

        assert ms.reference_channel == channel_names[0]

        ms.close()

    # ----- Read metadata test -----

    def test_read_data_matrix(self):
        ms = MedSession(self.session_path, self.level_2_password)
        dm = ms.data_matrix

        # Get required metadata
        start_time = ms.session_info['metadata']['start_time']
        end_time = ms.session_info['metadata']['end_time']

        # Make sure all channels are active
        ms.set_channel_active(ms.get_channel_names(), True)

        ref_channel = ms.reference_channel
        ref_channel_metadata = [x['metadata'] for x in ms.session_info['channels'] if x['metadata']['channel_name'] == ref_channel][0]
        ref_fs = ref_channel_metadata['sampling_frequency']
        ref_samples = ref_channel_metadata['absolute_end_sample_number'] - ref_channel_metadata['absolute_start_sample_number']

        # By index samples specified, sample major
        dm.major_dimension = 'sample'
        n_samples = 5000
        matrix_result = dm.get_matrix_by_index(0, int(10*ref_fs), None, n_samples)

        assert matrix_result['samples'].shape[0] == n_samples
        assert matrix_result['samples'].shape[1] == 4

        # By index frequency specified, channel major
        dm.major_dimension = 'channel'
        matrix_result = dm.get_matrix_by_index(0, int(10*ref_fs), 100, None)

        assert matrix_result['samples'].shape[0] == 4
        assert matrix_result['samples'].shape[1] == 1000

        # By time samples specified, sample major
        dm.major_dimension = 'sample'
        n_samples = 5000
        matrix_result = dm.get_matrix_by_time(start_time, start_time+10*1000000, None, n_samples)

        assert matrix_result['samples'].shape[0] == n_samples
        assert matrix_result['samples'].shape[1] == 4

        # By time frequency specified, channel major
        dm.major_dimension = 'channel'
        matrix_result = dm.get_matrix_by_time(start_time, start_time+10*1000000, 100, None)

        assert matrix_result['samples'].shape[0] == 4
        assert matrix_result['samples'].shape[1] == 1000

        # No start specified
        dm.major_dimension = 'channel'
        matrix_result = dm.get_matrix_by_time(None, start_time + 10 * 1000000, 100, None)

        assert matrix_result['samples'].shape[0] == 4
        assert matrix_result['samples'].shape[1] == 1000

        # No end specified
        dm.major_dimension = 'channel'
        matrix_result = dm.get_matrix_by_time(end_time - 10 * 1000000, None, 100, None)

        assert matrix_result['samples'].shape[0] == 4
        assert matrix_result['samples'].shape[1] == 1000

        # TODO: fix this - this currently fails
        # Nothing specified - time (i.e. read whole recording)
        # dm.major_dimension = 'channel'
        #
        # # channel_names = ms.get_channel_names()
        # # ms.set_channel_active(channel_names, False)
        # # ms.set_channel_active(channel_names[0], True)
        # # print("Reading data")
        # # matrix_result = dm.get_matrix_by_time(None, None, None, 5000)
        #
        # print(matrix_result['samples'].shape)
        #
        # assert matrix_result['samples'].shape[0] == 4
        # assert matrix_result['samples'].shape[1] == 5000

        ms.close()

    def test_read_session(self):
        ms = MedSession(self.session_path, self.level_2_password)

        # Get required metadata
        start_time = ms.session_info['metadata']['start_time']
        end_time = ms.session_info['metadata']['end_time']

        channel_names = ms.get_channel_names()
        channel_name = channel_names[0]
        channels_metadata = [x['metadata'] for x in ms.session_info['channels']]
        channel_metadata = [x['metadata'] for x in ms.session_info['channels'] if x['metadata']['channel_name'] == channel_name][0]
        chan_fs = channel_metadata['sampling_frequency']
        ref_channel = ms.reference_channel
        ref_index = channel_names.index(ref_channel)
        ref_channel_metadata = [channel_metadata['metadata'] for channel_metadata in ms.session_info['channels'] if channel_metadata['metadata']['channel_name'] == ref_channel][0]
        ref_n_samples = ref_channel_metadata['absolute_end_sample_number'] - ref_channel_metadata['absolute_start_sample_number']
        ref_fs = ref_channel_metadata['sampling_frequency']


        # Read by index - one channel specified
        data = ms.read_by_index(0, int(10 * chan_fs), channel_name)

        assert len(data) == int(10 * chan_fs)

        # Read by index - channels specified
        data = ms.read_by_index(0, int(10*chan_fs), channel_names)

        assert len(data) == len(channel_names)
        assert len(data[ref_index]) == int(10 * chan_fs)

        # Read by index - no channels specified
        data = ms.read_by_index(0, int(10*chan_fs), None)

        assert len(data) == len(channel_names)
        assert len(data[ref_index]) == int(10 * chan_fs)

        # Read by index - no start specified
        data = ms.read_by_index(None, int(10 * chan_fs))

        assert len(data) == len(channel_names)
        assert len(data[ref_index]) == int(10 * chan_fs)

        # Read by index - no end specified
        data = ms.read_by_index(ref_n_samples - int(10 * chan_fs), None, ref_channel)

        assert len(data) == int(10 * chan_fs)

        # Read by index - nothing specified
        data = ms.read_by_index(None, None)

        assert len(data) == len(channel_names)
        assert len(data[ref_index]) == ref_n_samples + 1

        # Read by time - one channel specified
        data = ms.read_by_time(start_time, start_time + 10 * 1000000, channel_name)

        assert len(data) == int(10 * chan_fs) +1

        # Read by time - channels specified
        data = ms.read_by_time(start_time, start_time + 10 * 1000000, channel_names)

        assert len(data) == len(channel_names)
        assert len(data[ref_index]) == int(10 * ref_fs) + 5 # TODO: why +5?

        # Read by time - no channels specified
        data = ms.read_by_time(start_time, start_time + 10 * 1000000)

        assert len(data) == len(channel_names)
        assert len(data[ref_index]) == int(10 * ref_fs) + 5

        # Read by time - no start specified
        data = ms.read_by_time(None, start_time + 10 * 1000000)

        assert len(data) == len(channel_names)
        assert len(data[ref_index]) == int(10 * ref_fs) + 5

        # Read by time - no end specified
        # data = ms.read_by_time(ref_channel_metadata['end_time'] - 10 * 1000000, None, ref_channel)

        # print("Read by time - no end specified")
        # print(len(data))

        # Read by time - nothing specified
        data = ms.read_by_time(None, None)

        assert len(data) == len(channel_names)
        assert len(data[ref_index]) == ref_n_samples + 1

        ms.close()

    # ----- Helpers -----

    def test_get_records(self):
        ms = MedSession(self.session_path, self.level_2_password)

        # records = ms.get_session_records()

        # print(records)

        ms.close()

    def test_get_contigua(self):
        ms = MedSession(self.session_path, self.level_2_password)

        channel_names = ms.get_channel_names()
        channel_name = channel_names[0]
        channel_metadata = [x['metadata'] for x in ms.session_info['channels'] if x['metadata']['channel_name'] == channel_name][0]
        channel_n_samples = channel_metadata['absolute_end_sample_number'] - channel_metadata['absolute_start_sample_number']
        ref_channel = ms.reference_channel
        ref_channel_metadata = [channels_metadata['metadata'] for channels_metadata in ms.session_info['channels'] if
                                channels_metadata['metadata']['channel_name'] == ref_channel][0]
        ref_n_samples = ref_channel_metadata['absolute_end_sample_number'] - ref_channel_metadata['absolute_start_sample_number']

        # Find contigua for current reference channel
        contigua = ms.find_discontinuities()

        assert contigua[0]['end_index'] == ref_n_samples - 1

        # TODO: session info contains number of samples for referential channel not the actual value for channel
        # Find contigua for a specific channel
        # contigua = ms.find_discontinuities(channel_name=channel_name)
        #
        # print(contigua)
        # print(channel_n_samples)

        # assert contigua[0]['end_index'] == channel_end_sample - 1

        ms.close()

    # def test_wrong_password(self):
    #     pass

if __name__ == '__main__':
    unittest.main()
