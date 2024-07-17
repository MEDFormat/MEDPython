# DHN-MED-Py

Python wrapper for MED format, GPL license 3.0.
Commercial exceptions to GPL open source requirements may be negotiated with Dark Horse Neuro, Inc.

Multiscale Electrophysiology Data Format (MED) is an open source data format developed to manage big data in electrophysiology and facilitate data sharing.

The MED format is maintained by [MEDFormat.org](https://medformat.org).

## Installation

To install please use:
```bash
pip install dhn-med-py
```
Numpy is a required dependency.

## Wrapper features

- Opens all MED format data for reading directly into python environment.
- Fully open-source (GPL license 3.0) for both C library code and python wrapper.
- Samples are returned in NumPy arrays for easy and efficient processing.
- Channel and session metadata are returned in python dictionaries.
- Threaded file opening and channel reading for optimal performance.
- Optional matrix (2D NumPy array) output for efficient processing/visualization.
- Support for major platforms (MacOS, Linux, Windows).
- Supported format in the [Neo project](https://github.com/NeuralEnsemble/python-neo/).

## Sample Script

```python
#!/usr/bin/env python3

import dhn_med_py
  
from dhn_med_py import MedSession

# open session
sess = MedSession("/Users/JohnDoe/Desktop/MED-test/RawData.medd", "password")

print("First channel name:", sess.session_info['channels'][0]['metadata']['channel_name'])
sampling_rate = sess.session_info['channels'][0]['metadata']['sampling_frequency']
print("Sampling rate of first channel:", sampling_rate)

# read first minute of data, in 1 second chunks
for y in range(0, 60):
    sess.read_by_index(sampling_rate * y, sampling_rate * (y+1))
    print(sess.data['channels'][0]['data'])

# read first minute of data, in 1 second chunks
# negative time means relative to beginning of session
for y in range(0, 60):
    sess.read_by_time(y * -1000000, (y+1) * -1000000)
    print(sess.data['channels'][0]['data'])
    
# read matrix of first 1 minute of data.
# Return 5000 samples of data per channel (matrix has 5000 columns)
# antialiasing will be applied when downsampling (default filter setting is 'antialias')
sess.get_matrix_by_time(0, -60 * 1000000, sample_count=5000)

# print number of samples in each channel of the resulting matrix
print(sess.matrix['sample_count'])

# print the resulting matrix samples (2D Numpy array)
print (sess.matrix['samples'])

# read matrix of first 1 minute of data
# Return a sampling frequency of 3000 Hz.
sess.get_matrix_by_time(0, -60 * 1000000, 3000)

# print resulting matrix
print (sess.matrix['samples'])

# Read entire dataset, with output sampling set to 1000 Hz
sess.get_matrix_by_time('start', 'end', 1000)

# Read first 25000 samples, using the channel "5k_0001" as the reference channel
# This means the first 5 seconds of the session (for all channels) will be read.
# The default output number of samples corresponds to the highest channel
# sampling frequency.
sess.set_reference_channel("5k_0001")
sess.get_matrix_by_index(0, 25000)

# helper function to set detrending (baseline correction) for future matrix calls
sess.set_detrend(True)

# helper function to set trace_ranges.  Matrix calls will return these
# as "minima" and "maxima" in the matrix result.
sess.set_trace_ranges(True)

# helper function to turn off filtering for future matrix calls
sess.set_filter("none")

# free session
del sess
```
