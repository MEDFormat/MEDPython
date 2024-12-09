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

#***********************************************************************//
#******************  DARK HORSE NEURO MED Python API  ******************//
#***********************************************************************//

# Written by Matt Stead, Dan Crepeau and Jan Cimbalnik
# Copyright Dark Horse Neuro Inc, 2023

# Local imports
from .medlib_flags import FLAGS
from .med_file.dhnmed_file import (
    initialize_session, initialize_data_matrix,
    set_session_capsule_destructor, set_data_matrix_capsule_destructor,
    remove_capsule_destructor,
    open_MED, read_MED,
    read_session_info,
    sort_channels_by_acq_num,
    set_channel_reference, get_channel_reference,
    get_globals_number_of_session_samples, find_discontinuities, get_session_records,
    read_lh_flags, push_lh_flags,
    read_dm_flags, push_dm_flags, get_dm
)


class MedDataMatrix:
    """

    """

    class InvalidArgumentException(Exception):
        pass

    __dm_capsule = None

    __valid_filters = ['none', 'antialias', 'lowpass', 'highpass', 'bandpass', 'bandstop']

    __valid_major_dimensions = ['channel', 'sample']

    def __init__(self, __sess_capsule):
        self.__sess_capsule = __sess_capsule

        # Set defaults for matrix operations
        self.__relative_indexing = True
        self.__return_records = True

        # Initialize data matrix
        self.__dm_capsule = initialize_data_matrix()

        # Set up default flags for matrix
        dm_flags = self._get_dm_flags()

        # Data type
        dm_flags['DM_TYPE_SF8_m12'] = True

        # Major dimension
        dm_flags['DM_FMT_CHANNEL_MAJOR_m12'] = True

        # Data operator flags
        dm_flags['DM_FILT_ANTIALIAS_m12'] = True
        dm_flags['DM_DETREND_m12'] = False
        dm_flags['DM_TRACE_RANGES_m12'] = False

        # Misc
        dm_flags['DM_EXTMD_RELATIVE_LIMITS_m12'] = True
        dm_flags['DM_DSCNT_CONTIG_m12'] = True
        dm_flags['DM_INTRP_UP_MAKIMA_DN_LINEAR_m12'] = True
        dm_flags['DM_EXTMD_SAMP_COUNT_m12'] = True

        self._set_dm_flags(dm_flags)

    # ----- Flag functions -----
    def _get_dm_flags(self):
        dm_flags = read_dm_flags(self.__dm_capsule)

        # Session level
        dm_flag_binary = dm_flags['data_matrix_flags']
        dm_session_flag_state = {}

        for f, pos in FLAGS['DM'].items():
            if dm_flag_binary[pos] == 1:
                dm_session_flag_state[f] = True
            else:
                dm_session_flag_state[f] = False

        return dm_session_flag_state

    def _set_dm_flags(self, dm_flags_python):
        dm_flags = {}

        # Session level flags
        flag_list = [0] * 64
        for flag, val in dm_flags_python.items():
            flag_list[FLAGS['DM'][flag]] = int(val)
        dm_flags['data_matrix_flags'] = flag_list

        push_dm_flags(self.__dm_capsule, dm_flags)

    # ----- Properties (exposed to user) -----

    @property
    def filter_type(self):
        dm_flags = self._get_dm_flags()

        if dm_flags['DM_FILT_ANTIALIAS_m12']:
            return 'antialias'
        elif dm_flags['DM_FILT_LOWPASS_m12']:
            return 'lowpass'
        elif dm_flags['DM_FILT_HIGHPASS_m12']:
            return 'highpass'
        elif dm_flags['DM_FILT_BANDPASS_m12']:
            return 'bandpass'
        elif dm_flags['DM_FILT_BANDSTOP_m12']:
            return 'bandstop'
        else:
            return 'none'

    @filter_type.setter
    def filter_type(self, filter_type):
        """
        Sets the filter to be used by the "matrix" operations.

        Filtering is done during get_matrix_by_index and get_matrix_by_time.

        The default filter setting is 'antialias', which is the minimum filtering that should be
        used when downsampling data.  In antialias mode, the antialias filter is only applied
        when downsampling occurs.

        Parameters
        ---------
        filter_type: str
            'none', 'antialias' are accepted values.

        Returns
        -------
        None
        """
        if not isinstance(filter_type, str):
            raise MedDataMatrix.InvalidArgumentException(
                "Invalid argument: filter_type must be a string.")
        if filter_type not in self.__valid_filters:
            raise MedDataMatrix.InvalidArgumentException(
                f"Invalid argument: filter_type must be one of the following {self.__valid_filters}.")
        dm_flags = self._get_dm_flags()

        if filter_type == 'antialias':
            dm_flags['DM_FILT_ANTIALIAS_m12'] = True
        else:
            dm_flags['DM_FILT_ANTIALIAS_m12'] = False

        if filter_type == 'lowpass':
            dm_flags['DM_FILT_LOWPASS_m12'] = True
        else:
            dm_flags['DM_FILT_LOWPASS_m12'] = False

        if filter_type == 'highpass':
            dm_flags['DM_FILT_HIGHPASS_m12'] = True
        else:
            dm_flags['DM_FILT_HIGHPASS_m12'] = False

        if filter_type == 'bandpass':
            dm_flags['DM_FILT_BANDPASS_m12'] = True
        else:
            dm_flags['DM_FILT_BANDPASS_m12'] = False

        if filter_type == 'bandstop':
            dm_flags['DM_FILT_BANDSTOP_m12'] = True
        else:
            dm_flags['DM_FILT_BANDSTOP_m12'] = False

        self._set_dm_flags(dm_flags)

    @property
    def detrend(self):

        dm_flags = self._get_dm_flags()

        return dm_flags['DM_DETREND_m12']

    @detrend.setter
    def detrend(self, detrend):
        """
        Sets the boolean to control detrend (baseline correction) generated by the "matrix" operations.

        Parameters
        ---------
        value: bool

        Returns
        -------
        None
        """
        dm_flags = self._get_dm_flags()

        dm_flags['DM_DETREND_m12'] = detrend

        self._set_dm_flags(dm_flags)

        return

    @property
    def trace_ranges(self):

        dm_flags = self._get_dm_flags()

        return dm_flags['DM_TRACE_RANGES_m12']

    @trace_ranges.setter
    def trace_ranges(self, trace_ranges):
        """
        Sets the boolean to control trace_ranges generated by the "matrix" operations.

        Trace ranges do not affect "read" operations, including read_by_index and read_by_time.
        Trace ranges can be calculated during get_matrix_by_index and get_matrix_by_time.

        Since matrix operations can potentially downsample, trace ranges can be used to show
        the max and min values actually present in the original signal.

        The matrix keys "minima" and "maxima" contain the trace ranges.

        Parameters
        ---------
        value: bool

        Returns
        -------
        None
        """
        dm_flags = self._get_dm_flags()

        dm_flags['DM_TRACE_RANGES_m12'] = trace_ranges

        self._set_dm_flags(dm_flags)

        return

    @property
    def major_dimension(self):

        dm_flags = self._get_dm_flags()

        if dm_flags['DM_FMT_CHANNEL_MAJOR_m12']:
            return 'channel'
        else:
            return 'sample'

    @major_dimension.setter
    def major_dimension(self, major_dimension):
        """
        Sets the major dimension to be returned by future "matrix" operations.

        The "samples" field of a matrix is a 2D NumPy array of 8 byte floating point values.
        The parameter to this function, "channel" or "sample", determines which is the outer
        array and which is the inner array.

        Example: If you have 2 signal channels, and 3 samples per channel, then the "samples"
        array of the matrix object would look like:

            "channel": [[a, b, c], [x, y, z]]
            "sample":  [[a, x], [b, y], [c, z]]

        "channel" is the default value when a new session is created.

        Note: this setting does not affect previously-generated matrices.  Previously
        generated matrix arrays can be reversed using the standard NumPy transpose() function.

        Parameters
        ---------
        major_dimension: str
            'channel', 'sample' are accepted values.

        Returns
        -------
        None
        """
        if not isinstance(major_dimension, str):
            raise MedDataMatrix.InvalidArgumentException(
                "Invalid argument: major_dimension must be a string.")
        if major_dimension not in self.__valid_major_dimensions:
            raise MedDataMatrix.InvalidArgumentException(
                f"Invalid argument: major_dimension must be one of the following {self.__valid_major_dimensions}.")
        dm_flags = self._get_dm_flags()

        if major_dimension == 'channel':
            dm_flags['DM_FMT_CHANNEL_MAJOR_m12'] = True
            dm_flags['DM_FMT_SAMPLE_MAJOR_m12'] = False
        else:
            dm_flags['DM_FMT_CHANNEL_MAJOR_m12'] = False
            dm_flags['DM_FMT_SAMPLE_MAJOR_m12'] = True

        self._set_dm_flags(dm_flags)

        return


    def get_matrix_by_time(self, start_time='start', end_time='end', sampling_frequency=None, sample_count=None):
        """
        Read all active channels of a MED session, by specifying start and end times.

        Times are specified in absolute uUTC (micro UTC) time, or negative times can be
        specified to refer to the beginning of the recording.  For example, reading the
        first 10 seconds of a session would look like:
        sess.get_matrix_by_time(0, -10 * 1000000, num_out_samps)

        Arguments 3 and 4 are sampling_frequency and sample_count, which refer to the size
        of the output matrix. At least one of them must be specified, but not both.

        This function returns a "matrix", which includes a "samples" array.  The array is a
        2-dimensional NumPy array, with the axes being channels and samples.  Such an array
        is optimized for viewer purposes.

        The default filter setting is 'antialias' which is applied when downsampling occurs.

        Parameters
        ---------
        start_time: int
            start_time is inclusive.
        end_time: int
            end_time is exclusive, per python conventions.
        sampling_frequency: float
            desired sampling frequency of output matrix
        sample_count: int
            number of output samples

        Returns
        -------
        matrix: dict
            matrix data structure is the output of this function.  A reference to this data is
            also stored in MedDataMatrix.matrix (class member variable).

            Contents of matrix dict are:
                start_time : int
                start_time_string : str
                end_time : int
                end_time_string : str
                channel_names : list of str
                channel_sampling_frequencies : list of floats
                contigua : list of contigua dicts (continuous data ranges)
                records : list of record dicts
                samples : 2D NumPy array
                minima : Numpy array or None
                maxima : Numpy array or None
                sampling_frequency : float
                sample_count : int
                channel_count : int
        """

        if (sampling_frequency is not None) and (sample_count is not None):
            raise MedSession.InvalidArgumentException(
                "Invalid arguments: sampling_frequency and sample_count can't both be specified.")

        dm_flags = self._get_dm_flags()
        if sampling_frequency is not None:
            dm_flags['DM_EXTMD_SAMP_FREQ_m12'] = True
            dm_flags['DM_EXTMD_SAMP_COUNT_m12'] = False
        else:
            dm_flags['DM_EXTMD_SAMP_FREQ_m12'] = False
            dm_flags['DM_EXTMD_SAMP_COUNT_m12'] = True
        self._set_dm_flags(dm_flags)

        self.matrix = get_dm(self.__sess_capsule,
                             self.__dm_capsule,
                             None, None,
                             start_time, end_time,
                             sample_count, sampling_frequency,
                             self.__return_records)

        return self.matrix

    def get_matrix_by_index(self, start_index, end_index, sampling_frequency=None, sample_count=None):
        """
        Read all active channels of a MED session, by specifying start and end sample indices.

        Indicies (or sample numbers) are referenced to a "reference channel" which can be
        specified in the constructor to MedSession, or using the set_reference_channel()
        function.  The default reference channel is the first channel in alphanumeric order.

        This function returns a "matrix", which includes a "samples" array.  The array is a
        2-dimensional NumPy array, with the axes being channels and samples.  Such an array
        is optimized for viewer purposes.

        The default filter setting is 'antialias' which is applied when downsampling occurs.

        Parameters
        ---------
        start_index: int
            start_index is inclusive.
        end_index: int
            end_index is exclusive, per python conventions.
        sampling_frequency: float
            desired sampling frequency of output matrix
        sample_count: int
            number of output samples

        Returns
        -------
        matrix: dict
            matrix data structure is the output of this function.  A reference to this data is
            also stored in MedSession.matrix (class member variable).

            Contents of matrix dict are:
                start_time : int
                start_time_string : str
                end_time : int
                end_time_string : str
                channel_names : list of str
                channel_sampling_frequencies : list of floats
                contigua : list of contigua dicts (continuous data ranges)
                records : list of record dicts
                samples : 2D NumPy array
                minima : Numpy array or None
                maxima : Numpy array or None
                sampling_frequency : float
                sample_count : int
                channel_count : int
        """

        if (sampling_frequency is not None) and (sample_count is not None):
            raise MedSession.InvalidArgumentException(
                "Invalid arguments: sampling_frequency and sample_count can't both be specified.")

        dm_flags = self._get_dm_flags()
        if sampling_frequency is not None:
            dm_flags['DM_EXTMD_SAMP_FREQ_m12'] = True
            dm_flags['DM_EXTMD_SAMP_COUNT_m12'] = False
        else:
            dm_flags['DM_EXTMD_SAMP_FREQ_m12'] = False
            dm_flags['DM_EXTMD_SAMP_COUNT_m12'] = True
        self._set_dm_flags(dm_flags)

        self.matrix = get_dm(self.__sess_capsule,
                             self.__dm_capsule,
                             start_index, end_index,
                             None, None,
                             sample_count, sampling_frequency,
                             self.__return_records)

        return self.matrix

    def close(self):

        set_data_matrix_capsule_destructor(self.__dm_capsule)

        if self.__dm_capsule is not None:
            self.__dm_capsule = None

        return


class MedSession:
    """
    Basic object for reading operations with MED sessions.
    
    The constructor opens the MED session (reads basic metadata and opens data files
    for reading).
    
    A structure called session_info is a member of the class: session_info is created
    when the session is opened.
    
    The destructor closes open files and frees allocated memory.

    Constructor Parameters
    ----------
    session_path: str, or list of str
        path to MED session (.medd), or a list of paths to MED channels (.ticd)
    password: str (default=None)
        password for MED session
    reference_channel (default=first channel in the session, in alphanumeric ordering)
        since different channels can have different sampling frequencies,
        select a particular channel to be used when indexing by sample number.
        
    Returns:
    ----------
    self.session_info: dict
        data structure that contains basic metadata info about the session.
            
        Contents of session_info are:
            metadata : metadata dict
            channels : list of channel dicts, contain info about each channel.
            contigua : list of contigua dicts (continuous data ranges)
            password_hints : list of str
    """
    
    class OpenSessionException(Exception):
        pass
        
    class BadPasswordException(Exception):
        pass
        
    class ReadSessionException(Exception):
        pass
        
    class InvalidArgumentException(Exception):
        pass
    
    __sess_capsule = None

    data_matrix = None



    def __init__(self, session_path, password=None, reference_channel=None):

        # Initialize session capsule
        self.__sess_capsule = initialize_session()

        # Set default for class destructor
        self.__close_on_destruct = True

        # Set default flags
        lh_flags = self._get_lh_flags()

        #lh_flags['session_level_lh_flags']['LH_INCLUDE_TIME_SERIES_CHANNELS_m12'] = True
        lh_flags['session_level_lh_flags']['LH_EXCLUDE_VIDEO_CHANNELS_m12'] = True
        lh_flags['session_level_lh_flags']['LH_MAP_ALL_TIME_SERIES_CHANNELS_m12'] = True
        lh_flags['session_level_lh_flags']['LH_MAP_ALL_SEGMENTS_m12'] = True
        lh_flags['session_level_lh_flags']['LH_READ_SLICE_SESSION_RECORDS_m12'] = True
        lh_flags['session_level_lh_flags']['LH_READ_SLICE_SEGMENTED_SESS_RECS_m12'] = True
        lh_flags['session_level_lh_flags']['LH_READ_SLICE_CHANNEL_RECORDS_m12'] = True
        lh_flags['session_level_lh_flags']['LH_READ_SLICE_SEGMENT_RECORDS_m12'] = True
        lh_flags['session_level_lh_flags']['LH_READ_SLICE_SEGMENT_DATA_m12'] = True
        lh_flags['session_level_lh_flags']['LH_GENERATE_EPHEMERAL_DATA_m12'] = True
        self._set_lh_flags(lh_flags)
      
        if password is not None:
            if not isinstance(password, str):
                raise MedSession.InvalidArgumentException("Invalid argument: password must be a string.")
                
        if reference_channel is not None:
            if not isinstance(reference_channel, str):
                raise MedSession.InvalidArgumentException("Invalid argument: reference channel must be a string.")

        open_MED(self.__sess_capsule, session_path, password)

        # this should never happen, but check for it anyway
        try:
            if self.__sess_capsule is None:
                raise MedSession.OpenSessionException("Unspecified error: Unable to open session: " + str(session_path))
        except:
            raise MedSession.OpenSessionException("Unspecified error: Unable to open session: " + str(session_path))

        if reference_channel is not None:
            self.reference_channel = reference_channel

        # read channel/session metadata
        self.session_info = read_session_info(self.__sess_capsule)

        # Create data matrix
        self.data_matrix = MedDataMatrix(self.__sess_capsule)
        
        return

    # ----- Private functions -----
    def _get_lh_flags(self):

        lh_flags = read_lh_flags(self.__sess_capsule)
        lh_flag_state = {}

        # Session level
        lh_flag_binary = lh_flags['session_flags']
        lh_session_flag_state = {}

        for f, pos in FLAGS['LH'].items():
            if lh_flag_binary[pos] == 1:
                lh_session_flag_state[f] = True
            else:
                lh_session_flag_state[f] = False

        lh_flag_state['session_level_lh_flags'] = lh_session_flag_state

        # Channel level
        lh_channels_flag_state = {}
        for channel, channel_dict in lh_flags['channels'].items():
            lh_channel_flag_state = {}
            lh_flag_binary = channel_dict['channel_flags']
            for f, pos in FLAGS['LH'].items():
                if lh_flag_binary[pos] == 1:
                    lh_channel_flag_state[f] = True
                else:
                    lh_channel_flag_state[f] = False
            lh_channels_flag_state[channel] = {'channel_level_lh_flags':
                                                lh_channel_flag_state}

            # Segment level
            lh_segments_flag_state = {}
            for segment, segment_dict in channel_dict['segments'].items():
                lh_segment_flag_state = {}
                lh_flag_binary = segment_dict['segment_flags']
                for f, pos in FLAGS['LH'].items():
                    if lh_flag_binary[pos] == 1:
                        lh_segment_flag_state[f] = True
                    else:
                        lh_segment_flag_state[f] = False
                lh_segments_flag_state[segment] = {'segment_level_lh_flags':
                                                    lh_segment_flag_state}

            lh_channels_flag_state[channel]['segments'] = lh_segments_flag_state

        lh_flag_state['channels'] = lh_channels_flag_state

        return lh_flag_state

    def _set_lh_flags(self, lh_flags_python):
        """
        Set session/channel/segment LH level flags.
        Args:
            lh_flags_python:

        Returns:

        """

        lh_flags = {}

        # Session level flags
        flag_list = [0]*64
        for flag, val in lh_flags_python['session_level_lh_flags'].items():
            flag_list[FLAGS['LH'][flag]] = int(val)

        lh_flags['session_flags'] = flag_list

        lh_flags['channels'] = {}
        for channel, channel_dict in lh_flags_python['channels'].items():
            # Channel level flags
            flag_list = [0]*64
            for flag, val in channel_dict['channel_level_lh_flags'].items():
                flag_list[FLAGS['LH'][flag]] = int(val)
            lh_flags['channels'][channel] = {'channel_flags': flag_list}

            lh_flags['channels'][channel]['segments'] = {}
            for segment, segment_dict in channel_dict['segments'].items():
                # Segment level flags
                flag_list = [0]*64
                for flag, val in segment_dict['segment_level_lh_flags'].items():
                    flag_list[FLAGS['LH'][flag]] = int(val)
                lh_flags['channels'][channel]['segments'][segment] = {'segment_flags': flag_list}

        push_lh_flags(self.__sess_capsule, lh_flags)

        return None

    def get_channel_names(self):
        channel_names = []
        for channel in self.session_info['channels']:
            channel_names.append(channel['metadata']['channel_name'])
        return channel_names

    def read_by_time(self, start_time, end_time, channels=None):
        """
        Read all active channels of a MED session, by specifying start and end times.
        
        Times are specified in microseconds UTC (uUTC), either relative to the beginning of the session,
        or in absolute epoch terms (since 1 Jan 1970).
        Positive times are considered to be absolute, and negative times are considered to be relative.
        
        Examples of relative times:
            First second of a session:
                start: 0, end: -1000000
            Second minute of a session:
                start: -60000000, end: -120000000
        
        Parameters
        ---------
        start_time: int or None
            start_time is inclusive.
            see note above on absolute vs. relative times
        end_time: int or None
            end_time is exclusive, per python conventions.
        channels: str or list
            Single channel or list of channels to read.  If not specified, then all active channels will be read.

        Returns
        -------
        data: dict
            data structure is the output of this function.  A reference to this data is also stored in
            MedSession.data (class member variable).
            
            Contents of data are:
                metadata : metadata dict
                channels : list of channel dicts
                records : list of record dicts
                password_hints : list of str
        """
    
        if self.__sess_capsule is None:
            raise MedSession.ReadSessionException("Unable to read session!  Session is invalid.")

        # Check inputs
        if start_time is not None:
            if type(start_time) is not int:
                raise MedSession.ReadSessionException("start_time must be an int or None")

        if end_time is not None:
            if type(end_time) is not int:
                raise MedSession.ReadSessionException("end_time must be an int or None")

        # Activate specified channels
        if channels is not None:
            channel_names = self.get_channel_names()
            curr_active_channels = []
            lh_flags = self._get_lh_flags()
            for channel in channel_names:
                if lh_flags['channels'][channel]['channel_level_lh_flags']['LH_CHANNEL_ACTIVE_m12'] is True:
                    curr_active_channels.append(channel)
            self.set_channel_active(channel_names, False)
            self.set_channel_active(channels, True)

            data = read_MED(self.__sess_capsule, start_time, end_time, None, None)

            # Make sure the data is in requested channels order
            if type(channels) is list:
                channel_indices = [channel_names.index(x) for x in channels]
                reordered_pos = sorted(range(len(channel_indices)), key=lambda x: channel_indices[x])
                data = [data[i] for i in reordered_pos]

            self.set_channel_active(channel_names, False)
            self.set_channel_active(curr_active_channels, True)
        else:
            data = read_MED(self.__sess_capsule, start_time, end_time, None, None)

        # If only one channel was read, return array directly
        if type(channels) is str:
            data = data[0]

        return data
        
    def read_by_index(self, start_idx, end_idx, channels=None):
        """
        Read all active channels of a MED session, by specifying start and end sample numbers.
        
        Sample numbers are relative to a reference channel, which is specified by an optional
        parameter in the constructor.  If no reference channel is specified, then the default is
        the first channel (in alphanumeric channel name).  A reference channel is necessary
        because different channels can have different sampling frequencies, and the same amount
        of time is read for all channels by this function (sample numbers are converted to
        timestamps for the purposes of this function).
        
        Parameters
        ---------
        start_idx: int or None
            start_idx is inclusive.
        end_idx: int or None
            end_idx is exclusive, per python conventions.
        channels: str or list
            Single channel or list of channels to read.  If not specified, then all active channels will be read.

        Returns
        -------
        data : dict
            data structure is the output of this function.  A reference to this data is also stored in
            MedSession.data (class member variable).
            
            Contents of data are:
                metadata : metadata dict
                channels : list of channel dicts
                records : list of record dicts
                password_hints : list of str
        """
    
        if self.__sess_capsule is None:
            raise MedSession.ReadSessionException("Unable to read session!  Session is invalid.")

        # Check inputs
        if start_idx is not None:
            if type(start_idx) is not int:
                raise MedSession.ReadSessionException("start_idx must be an int or None")

        if end_idx is not None:
            if type(end_idx) is not int:
                raise MedSession.ReadSessionException("end_idx must be an int or None")

        if type(channels) is str:
            if channels not in self.get_channel_names():
                raise MedSession.ReadSessionException("Invalid channel name specified.")

        # Activate specified channels
        if channels is not None:
            channel_names = self.get_channel_names()
            curr_active_channels = []
            lh_flags = self._get_lh_flags()
            for channel in channel_names:
                if lh_flags['channels'][channel]['channel_level_lh_flags']['LH_CHANNEL_ACTIVE_m12'] is True:
                    curr_active_channels.append(channel)
            self.set_channel_active(channel_names, False)
            self.set_channel_active(channels, True)

            data = read_MED(self.__sess_capsule, None, None, start_idx, end_idx)

            # Make sure the data is in requested channels order
            if type(channels) is list:
                channel_indices = [channel_names.index(x) for x in channels]
                reordered_pos = sorted(range(len(channel_indices)), key=lambda x: channel_indices[x])
                data = [data[i] for i in reordered_pos]

            self.set_channel_active(channel_names, False)
            self.set_channel_active(curr_active_channels, True)
        else:
            data = read_MED(self.__sess_capsule, None, None, start_idx, end_idx)

        # If only one channel was read, return array directly
        if type(channels) is str:
            data = data[0]

        return data
        
    def sort_chans_by_acq_num(self):
        """
        Re-orders channels by acquisition_channel_number, lowest to highest.
        
        Any future reads (read_by_time, read_by_index, get_raw_page) will use this new ordering for
        the channel array.
        
        Returns
        -------
        None
        """
    
        sort_channels_by_acq_num(self.__sess_capsule)
        
        # read channel/session metadata
        #self.session_info = read_session_info(self.__sess_capsule)
        
        return

    def set_channel_active(self, chan_name, is_active=True):
        """
        Sets the specified channel (or list of channels) to be active (default) or inactive.
   
        An active channel is a channel that is used in read operations.  If a session has a lot
        of channels, then it might be useful to make a subset inactive, so we can just read
        from the remaining subset of channels.
        
        The function set_channel_inactive is identical to this function if the boolean value is
        false.  For example, the following two function calls do the same thing:
            sess.set_channel_active("channel_001", False)
            sess.set_channel_inactive("channel_001")
        set_channel_inactive is provided as a convenience.
        
        The keyword "all" can be used to specify all channels.  "all" cannot be a string in a
        list of channels.
        
        A warning is generated if a channel is deactivated that is the reference channel.  In
        this case the reference channel is not modified - but will be if a read call is made
        using index values.  So the burden is on the user to specify what a new reference channel
        should be.
        
        Channel names are case-sensitive.
        
        Parameters
        ---------
        chan_name: str, or list of str
            name of channel to activate or inactivate, or a list of channels.
        is_active : bool
            defaults to True (setting channel to be active).
        
        Returns
        -------
        None
        """
        lh_flags = self._get_lh_flags()
        if type(chan_name) is list:
            for chan in chan_name:
                if type(chan) is not str:
                    raise MedSession.InvalidArgumentException("List argument must be a list of strings.")
            for chan in chan_name:
                if chan not in lh_flags['channels'].keys():
                    raise MedSession.InvalidArgumentException("Channel name not found in session.")
                lh_flags['channels'][chan]['channel_level_lh_flags']['LH_CHANNEL_ACTIVE_m12'] = is_active
        elif type(chan_name) is str:
            if chan_name not in lh_flags['channels'].keys():
                raise MedSession.InvalidArgumentException("Channel name not found in session.")
            lh_flags['channels'][chan_name]['channel_level_lh_flags']['LH_CHANNEL_ACTIVE_m12'] = is_active
        else:
            raise MedSession.InvalidArgumentException("Argument must be either a list or a string.")

        self._set_lh_flags(lh_flags)

        # self.session_info = read_session_info(self.__sess_capsule)
        
        return

    @property
    def reference_channel(self):
        """
        Sets the reference channel to be the string specified.

        In general, reference values are used when reading across many channels, but the channels
        have different sampling frequencies.

        For example, If channel 1 has a frequency of 5000 Hz, and channel 2 has a frequency of 10000 Hz,
        then if you read from sample 0 to 4999, you will receive either 1 second or 2 seconds of data,
        depending on which channel is the reference channel.

        Reference channels are not used when using timestamps to specify start/end ranges for data
        reading.
        """

        return get_channel_reference()

    @reference_channel.setter
    def reference_channel(self, chan_name):
        """
        Sets the reference channel to be the string specified.

        In general, reference values are used when reading across many channels, but the channels
        have different sampling frequencies.

        For example, If channel 1 has a frequency of 5000 Hz, and channel 2 has a frequency of 10000 Hz,
        then if you read from sample 0 to 4999, you will receive either 1 second or 2 seconds of data,
        depending on which channel is the reference channel.

        Reference channels are not used when using timestamps to specify start/end ranges for data
        reading.
        """

        if not isinstance(chan_name, str):
            raise MedSession.InvalidArgumentException("Argument must be a string.")

        set_channel_reference(self.__sess_capsule, chan_name)
        
    def get_globals_number_of_session_samples(self):
        """
        This returns the number of samples in a session, assuming a reference channel has
        been set prior to calling it.
        
        This function is useful when a session has been opened for reading but no data has
        yet been read.  This is a quick and easy way to find out how many total samples are
        in the reference channel of the session.
        
        Parameters
        ---------
        None
        
        Returns
        -------
        value: int
        """
    
        return get_globals_number_of_session_samples(self.__sess_capsule)
        
    def find_discontinuities(self, channel_name=None):
        """
        This function returns a contigua (list of continuous data ranges).
        Each continuous range dictionary has the following elements:
            
            start_sample_number
            end_sample_number
            start_time
            end_time
        
        The sample numbers are determined by which channel is being used as
        the reference channel.  The reference channel ,which should be explicitly
        set prior to calling this function, can be set with the
        set_reference_channel() function.
        
        Parameters
        ---------
        None
        
        Returns
        -------
        contigua : list of contigua dicts (continuous data ranges)
        """

        if channel_name is not None:
            original_ref_channel = self.reference_channel
            self.reference_channel = channel_name
            contigua = find_discontinuities(self.__sess_capsule)
            self.reference_channel = original_ref_channel
            return contigua
        else:
            return find_discontinuities(self.__sess_capsule)
        
    def get_session_records(self, start_time=None, end_time=None):
        """
        This function returns a list of records corresponding to the time
        constraints of start_time and end_time.
        
        Each returned record dictionary 
        
        Parameters
        ---------
        start_time: int or None
            start_time is inclusive.
            see note above on absolute vs. relative times
        end_time: int or None
            end_time is exclusive, per python conventions.
        
        Returns
        -------
        records : list of record dicts
        """
    
        return get_session_records(self.__sess_capsule, start_time, end_time)

    @property
    def close_on_destruct(self):
        """
        Returns the boolean to control session closure on object destruction.

        Parameters
        ---------
        None

        Returns
        -------
        value: bool
        """

        return self.__close_on_destruct

    @close_on_destruct.setter
    def close_on_destruct(self, value):
        """
        Sets the boolean to control session closure on object destruction.

        Parameters
        ---------
        value: bool

        Returns
        -------
        None
        """

        if type(value) != bool:
            raise MedSession.InvalidArgumentException("Argument must be a boolean.")

        if value is True:
            set_session_capsule_destructor(self.__sess_capsule)
            dm = self.data_matrix
            set_data_matrix_capsule_destructor(dm.__dm_capsule)
        else:
            remove_capsule_destructor(self.__sess_capsule)
            dm = self.data_matrix
            remove_capsule_destructor(dm.__dm_capsule)

        self.__close_on_destruct = value

    def close(self):

        if self.data_matrix is not None:
            self.data_matrix.close()

        set_session_capsule_destructor(self.__sess_capsule)

        if self.__sess_capsule is not None:
            self.__sess_capsule = None

        return

    def __del__(self):
    
        if self.__sess_capsule is not None and self.__close_on_destruct is True:
            self.close()
        return

