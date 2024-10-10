// This file is part of the DHN-MED-Py distribution.
// Copyright (c) 2023 Dark Horse Neuro Inc.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 3.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

//***********************************************************************//
//******************  DARK HORSE NEURO MED Python API  ******************//
//***********************************************************************//

// Written by Matt Stead, Dan Crepeau and Jan Cimbalnik
// Copyright Dark Horse Neuro Inc, 2023

#include "dhnmed_file.h"

#include <numpy/arrayobject.h>
#include <numpy/npy_math.h>

#include "medlib_m12.c"
#include "medrec_m12.c"


si1*    check_utf8(si1* input_string) {
    if (UTF8_is_valid_m12(input_string, FALSE_m12, NULL) == TRUE_m12)
        return input_string;
    else
        return "<unreadable>";
}


static PyObject *test_api(PyObject *self, PyObject *args)
{
   /* si8 timestamp;
    char output_statement[1024];
    
    // initialize MEF library
    (void) initialize_medlib_m12(FALSE_m12, FALSE_m12);
    
    globals_m12->recording_time_offset = 10;
    
    timestamp = 22;
    remove_recording_time_offset_m12(&timestamp);
    
    sprintf(output_statement, "Offsetted timestamp is %ld.\n", timestamp);
    
    PyErr_WarnEx(PyExc_RuntimeWarning, output_statement, 1);
    Py_INCREF(Py_None);*/
    
    printf("Test API successful!\n");
    
    Py_INCREF(Py_None);
    Py_RETURN_NONE;
}

void dm_capsule_destructor(PyObject *capsule) {
    void *dm = PyCapsule_GetPointer(capsule, PyCapsule_GetName(capsule));
    if (dm != NULL) {
        DM_free_matrix_m12(dm, TRUE_m12);
    }
}

void session_capsule_destructor (PyObject *capsule){
    PyCapsule_SetDestructor(capsule, NULL);
    void *sess = PyCapsule_GetPointer(capsule, PyCapsule_GetName(capsule));
    if (sess != NULL) {
        G_free_session_m12(sess, TRUE_m12);
    }
    if (globals_m12 != NULL) {
        G_free_globals_m12(TRUE_m12);
    }
}

PyObject    *set_session_capsule_destructor (PyObject *self, PyObject *args){

    PyObject                *sess_capsule_object;

    // --- Parse the input ---
    if (!PyArg_ParseTuple(args,"O",
                          &sess_capsule_object)){

        PyErr_SetString(PyExc_RuntimeError, "Session capsule required\n");
        PyErr_Occurred();
        return NULL;
    }

    PyCapsule_SetDestructor(sess_capsule_object, session_capsule_destructor);

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject    *set_data_matrix_capsule_destructor (PyObject *self, PyObject *args){

    PyObject                *dm_capsule_object;

    // --- Parse the input ---
    if (!PyArg_ParseTuple(args,"O",
                          &dm_capsule_object)){

        PyErr_SetString(PyExc_RuntimeError, "Data matrix capsule required\n");
        PyErr_Occurred();
        return NULL;
    }

    PyCapsule_SetDestructor(dm_capsule_object, dm_capsule_destructor);

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject    *remove_capsule_destructor (PyObject *self, PyObject *args){


    PyObject                *capsule_object;

    // --- Parse the input ---
    if (!PyArg_ParseTuple(args,"O",
                          &capsule_object)){

        PyErr_SetString(PyExc_RuntimeError, "Capsule object required\n");
        PyErr_Occurred();
        return NULL;
    }

    PyCapsule_SetDestructor(capsule_object, NULL);

    Py_INCREF(Py_None);
    return Py_None;
}


PyObject            *initialize_data_matrix(PyObject *self, PyObject *args)
{
    DATA_MATRIX_m12         *dm;

    dm = (DATA_MATRIX_m12 *) calloc_m12((size_t) 1, sizeof(DATA_MATRIX_m12), __FUNCTION__, USE_GLOBAL_BEHAVIOR_m12);
    return PyCapsule_New((void *) dm, "dm", dm_capsule_destructor);

}

PyObject            *initialize_session(PyObject *self, PyObject *args)
{
    SESSION_m12         *sess;

    // initialize MED library
    G_initialize_medlib_m12(FALSE_m12, FALSE_m12);

    sess = (SESSION_m12 *) calloc_m12((size_t) 1, sizeof(SESSION_m12), __FUNCTION__, USE_GLOBAL_BEHAVIOR_m12);
    return PyCapsule_New((void *) sess, "session", session_capsule_destructor);
}

void    *initialize_time_slice(TIME_SLICE_m12 *slice, PyObject *start_index_obj, PyObject *end_index_obj, PyObject *start_time_obj, PyObject *end_time_obj)
{

    si8                                     start_time, end_time, start_index, end_index;

    PyObject                                *number;


//    PyObject    *temp_UTF_str;
//    si1         *temp_str_bytes;
//    printf("Start index is %d\n", start_index);

    // Set default values
    start_time = end_time = UUTC_NO_ENTRY_m12;
    start_index = end_index = SAMPLE_NUMBER_NO_ENTRY_m12;

    // Check if input objects are NULL and raise error if they are
    if (start_index_obj == NULL){
        PyErr_SetString(PyExc_RuntimeError, "Start index object cannot be NULL\n");
        PyErr_Occurred();
        return NULL;
    }

    if (end_index_obj == NULL){
        PyErr_SetString(PyExc_RuntimeError, "End index object cannot be NULL\n");
        PyErr_Occurred();
        return NULL;
    }

    if (start_time_obj == NULL){
        PyErr_SetString(PyExc_RuntimeError, "Start time object cannot be NULL\n");
        PyErr_Occurred();
        return NULL;
    }

    if (end_time_obj == NULL){
        PyErr_SetString(PyExc_RuntimeError, "End time object cannot be NULL\n");
        PyErr_Occurred();
        return NULL;
    }

    // Indices and times are mutually exclusive
    if ((start_index_obj != Py_None || end_index_obj != Py_None) && (start_time_obj != Py_None || end_time_obj != Py_None)){
        PyErr_SetString(PyExc_RuntimeError, "Specifying indices and times is mutually exclusive\n");
        PyErr_Occurred();
        return NULL;
    }

    if (start_index_obj == Py_None){
        if (start_time_obj == Py_None && end_time_obj == Py_None && end_index_obj != Py_None)
            start_index = BEGINNING_OF_SAMPLE_NUMBERS_m12;
        }
    else if (PyNumber_Check(start_index_obj))
        start_index = (si8) PyLong_AsLongLong(start_index_obj);
    else {
        PyErr_SetString(PyExc_RuntimeError, "Start Index must be an integer or None\n");
        PyErr_Occurred();
        return NULL;
    }

    if (end_index_obj == Py_None){
        if (start_time_obj == Py_None && end_time_obj == Py_None && start_index_obj != Py_None)
            end_index = END_OF_SAMPLE_NUMBERS_m12;
        }
    else if (PyNumber_Check(end_index_obj)){
        end_index = (si8) PyLong_AsLongLong(end_index_obj);
        end_index = end_index - 1;   //subtract one, since in Python end values are exclusive
        }
    else {
        PyErr_SetString(PyExc_RuntimeError, "End Index must be an integer or None\n");
        PyErr_Occurred();
        return NULL;
    }

    if (start_time_obj == Py_None){
        if (start_index_obj == Py_None && end_index_obj == Py_None)
            start_time = BEGINNING_OF_TIME_m12;
        else
            start_time = UUTC_NO_ENTRY_m12;
        }
    else if (PyNumber_Check(start_time_obj))
        start_time = (si8) PyLong_AsLongLong(start_time_obj);
    else {
        PyErr_SetString(PyExc_RuntimeError, "Start Time must be an integer or None\n");
        PyErr_Occurred();
        return NULL;
    }

    if (end_time_obj == Py_None){
        if (start_index_obj == Py_None && end_index_obj == Py_None)
            end_time = END_OF_TIME_m12;
        else
            end_time = UUTC_NO_ENTRY_m12;
        }
    else if (PyNumber_Check(end_time_obj)){
        end_time = (si8) PyLong_AsLongLong(end_time_obj);
        // make end_time not inclusive, by adjust by 1 microsecond.
        if (end_time > 0)
            end_time = end_time - 1;
        else
            end_time = end_time + 1;
        }
    else {
        PyErr_SetString(PyExc_RuntimeError, "End Time must be an integer or None\n");
        PyErr_Occurred();
        return NULL;
    }

    // Set values
    slice->start_time = start_time;
    slice->end_time = end_time;
    slice->start_sample_number = start_index;
    slice->end_sample_number = end_index;

//    G_show_time_slice_m12(slice);
}


static PyObject *open_MED(PyObject *self, PyObject *args)
{
    SESSION_m12                             *sess;
    si1                     *file_list, **file_list_p;
    si1                     *temp_str_bytes;
    PyObject                *temp_UTF_str;
    si4                     i, n_files;
    si1                     password[PASSWORD_BYTES_m12];
    PyObject                *password_input_obj;
    PyObject                *file_list_seq_obj;
    PyObject* file_list_obj;
    TERN_m12     license_ok;
    TIME_SLICE_m12    slice;
    si1                     pwd_hint_str[256];
    si8                     *py_pointer_changed;
    PyObject                *sess_capsule_object;
    
    password_input_obj = NULL;

    // --- Parse the input ---
    if (!PyArg_ParseTuple(args,"OO|O",
                          &sess_capsule_object,
                          &file_list_obj,
                          &password_input_obj)){
        
        PyErr_SetString(PyExc_RuntimeError, "One to 3 inputs required: file_list, [password], [reference_channel]\n");
        PyErr_Occurred();
        return NULL;
    }
    PROC_adjust_open_file_limit_m12(MAX_OPEN_FILES_m12(1024, 1), TRUE_m12);

    sess = PyCapsule_GetPointer(sess_capsule_object, "session");

    if (sess == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid session pointer");
        PyErr_Occurred();
        return NULL;
    }

    if (PyUnicode_Check(file_list_obj)) {
        file_list = calloc_m12((size_t) FULL_FILE_NAME_BYTES_m12, sizeof(si1), __FUNCTION__, USE_GLOBAL_BEHAVIOR_m12);
        temp_UTF_str = PyUnicode_AsEncodedString(file_list_obj, "utf-8","strict"); // Encode to UTF-8 python objects
        temp_str_bytes = PyBytes_AS_STRING(temp_UTF_str); // Get the *char
        strcpy(file_list,temp_str_bytes);
        n_files = 0;  // "make list_len zero to indicate a one dimention char array" (from medlib_m12.c)
    } else {
        file_list_seq_obj = PySequence_Fast(file_list_obj, "Expected a tuple or list for start." );

        n_files = PySequence_Fast_GET_SIZE(file_list_seq_obj);

        if (n_files > 1) {
            file_list = (void *) calloc_2D_m12((size_t) n_files, (size_t) FULL_FILE_NAME_BYTES_m12, sizeof(si1), __FUNCTION__, USE_GLOBAL_BEHAVIOR_m12);
            file_list_p =  (si1 **) file_list;
            for (i = 0; i < n_files; i++) {
 
                temp_UTF_str = PyUnicode_AsEncodedString(PySequence_Fast_GET_ITEM(file_list_seq_obj, i), "utf-8","strict"); // Encode to UTF-8 python objects
                temp_str_bytes = PyBytes_AS_STRING(temp_UTF_str);
                strcpy(file_list_p[i],temp_str_bytes);
                printf("file %d, %s\n", i, file_list_p[i]);
            }
        }
        if (n_files == 1) {

            file_list = calloc_m12((size_t) FULL_FILE_NAME_BYTES_m12, sizeof(si1), __FUNCTION__, USE_GLOBAL_BEHAVIOR_m12);
            temp_UTF_str = PyUnicode_AsEncodedString(PySequence_Fast_GET_ITEM(file_list_seq_obj, 0), "utf-8","strict"); // Encode to UTF-8 python objects
            temp_str_bytes = PyBytes_AS_STRING(temp_UTF_str); // Get the *char
            strcpy(file_list,temp_str_bytes);
            n_files = 0;
        }
        Py_DECREF(file_list_seq_obj);
    }
    
    // parse password input into string
    if (password_input_obj != NULL) {
        if (PyUnicode_Check(password_input_obj)) {
            
            temp_UTF_str = PyUnicode_AsEncodedString(password_input_obj, "utf-8","strict"); // Encode to UTF-8 python objects
            temp_str_bytes = PyBytes_AS_STRING(temp_UTF_str); // Get the *char
            
            if (!*temp_str_bytes) {
                password[0] = 0;
            } else {
                //level_2_password = calloc(PASSWORD_BYTES_m12, sizeof(si1));
                strcpy(password,temp_str_bytes);
            }
        } else {
            password[0] = 0;
        }
    } else {
        password[0] = 0;
    }

    globals_m12->behavior_on_fail = RETURN_ON_FAIL_m12;
    G_initialize_time_slice_m12(&slice);  // this defaults to full session if nothing specified

    slice.start_time = BEGINNING_OF_TIME_m12;
    slice.end_time = END_OF_TIME_m12;

    sess = G_open_session_m12(sess, &slice, file_list, n_files, 0, password);

    if (sess == NULL) {
		if (globals_m12->password_data.processed == 0) {
		    PyErr_SetString(PyExc_RuntimeError, "No matching input files");
            PyErr_Occurred();
            Py_INCREF(Py_None);
            return Py_None;
		} else {
			if (*globals_m12->password_data.level_1_password_hint || *globals_m12->password_data.level_2_password_hint) {
                PyErr_Format(PyExc_RuntimeError, "Invalid password. Level 1 password hint: %s. Level 2 password hint: %s", globals_m12->password_data.level_1_password_hint, globals_m12->password_data.level_2_password_hint);
                PyErr_Occurred();
                Py_INCREF(Py_None);
                return Py_None;
			} else {
			    PyErr_SetString(PyExc_RuntimeError, "Metadata file does not exist");
                PyErr_Occurred();
                Py_INCREF(Py_None);
                return Py_None;
			}
		}

		Py_INCREF(Py_None);
        return Py_None;
	}

    // TODO: there was a memory leak here - verify

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *read_session_info(PyObject *self, PyObject *args)
{
    PyObject                                *py_info;
    TIME_SLICE_m12                          *chan_slice;
    PyObject                                *py_metadata, *py_channels;
    PyObject                                *py_channel;
    PyObject                                *py_channel_metadata, *py_contigua, *py_password_hints;
    PyObject                                *py_contiguon;
    CHANNEL_m12                             *chan;
    TIME_SERIES_METADATA_SECTION_2_m12      *sess_tmd2, *chan_tmd2;
    si8                                     i, k;
    si4                                     n_channels, n_active_chans;
    SESSION_m12         *sess;
    PyObject* item;
    PyObject* seq;
    PyObject                *sess_capsule_object;
    si8             n_contigua;
    si8             current_start_index, current_end_index, current_start_time, current_end_time;

    
    
    // this assumes the channel has already been opened
    
    // --- Parse the input ---
    if (!PyArg_ParseTuple(args,"O",
                          &sess_capsule_object)){
        
        PyErr_SetString(PyExc_RuntimeError, "One  inputs required: pointers\n");
        PyErr_Occurred();
        return NULL;
    }
    
    sess = (SESSION_m12 *) PyCapsule_GetPointer(sess_capsule_object, "session");

    if (sess == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Error reading session pointer\n");
        PyErr_Occurred();
        return NULL;
    }

    //show_time_slice_m12(&sess->time_slice);

//    G_show_Sgmt_records_array_m12(sess);
    n_contigua = G_build_contigua_m12((LEVEL_HEADER_m12 *) sess);
    //printf("n_contigs = %d\n", n_contigs);
    
    //     ******************************************
    //     ********  Create Python structure  *******
    //     ******************************************

    py_info = Py_None;
    py_metadata = Py_None;
    py_channels = Py_None;
    py_contigua = Py_None;
    py_password_hints = Py_None;
    
    n_channels = sess->number_of_time_series_channels;
    
    for (i = n_active_chans = 0; i < sess->number_of_time_series_channels; ++i) {
        chan = sess->time_series_channels[i];
        if (chan->flags & LH_CHANNEL_ACTIVE_m12)
            ++n_active_chans;
    }

    py_contigua = PyList_New(n_contigua);
    sess_tmd2 = &sess->time_series_metadata_fps->metadata->time_series_section_2;
    for (i=0; i<n_contigua; ++i) {
        //py_contiguon_read = PyList_GetItem(first_chan_contigua, i);
        if (sess_tmd2->sampling_frequency == FREQUENCY_VARIABLE_m12) {
            current_start_index = -1;
            current_end_index = -1;
        } else {
            current_start_index = sess->contigua[i].start_sample_number;
            current_end_index = sess->contigua[i].end_sample_number;
        }
        current_start_time = sess->contigua[i].start_time;
        current_end_time = sess->contigua[i].end_time;
        py_contiguon = Py_BuildValue("{s:L,s:L,s:L,s:L}",
                                     "start_index", current_start_index,
                                     "end_index", current_end_index,
                                     "start_time", current_start_time,
                                     "end_time", current_end_time);
        //printf("Got here 2a\n");
        PyList_SetItem(py_contigua, i, py_contiguon);
    }


    // Create channel sub-structures
    py_channels = PyList_New(n_active_chans);
    //printf("n_channels: %d\n", n_channels);
    for (i = k = 0; k< n_channels; ++k) {

        chan = sess->time_series_channels[k];
        if ((chan->flags & LH_CHANNEL_ACTIVE_m12) == 0)
            continue;
        
        chan_slice = &chan->time_slice;
        //show_time_slice_m12(chan_slice);
        chan_tmd2 = &chan->metadata_fps->metadata->time_series_section_2;
        
        // Create session metadata output structure
        sess_tmd2 = &sess->time_series_metadata_fps->metadata->time_series_section_2;
        if (sess_tmd2->sampling_frequency != FREQUENCY_VARIABLE_m12 && sess_tmd2->sampling_frequency != FREQUENCY_NO_ENTRY_m12) {
            chan_tmd2 = &sess->time_series_channels[0]->metadata_fps->metadata->time_series_section_2;
            sess_tmd2->absolute_start_sample_number = chan_tmd2->absolute_start_sample_number;
            //printf("DEBUG113: %ld\n", sess->time_series_channels[0]->metadata_fps->metadata->time_series_section_2.number_of_samples);

            sess_tmd2->number_of_samples = chan_tmd2->number_of_samples;
           // printf("DEBUG114: %ld\n", sess->time_series_channels[0]->metadata_fps->metadata->time_series_section_2.number_of_samples);

        }
        
        //printf("DEBUG: %ld\n", chan_tmd2->number_of_samples);
        
        // fill in channel metadata
        chan_tmd2 = &chan->metadata_fps->metadata->time_series_section_2;
        
        //chan_tmd2->absolute_start_sample_number = chan_slice->start_sample_number;
        //chan_tmd2->number_of_samples = TIME_SLICE_SAMPLE_COUNT_m12(chan_slice);
        //printf("DEBUG: %ld\n", chan_tmd2->number_of_samples);
        py_channel_metadata = fill_metadata(chan->metadata_fps, chan_slice);
       
        py_channel = Py_BuildValue("{s:O}", "metadata", py_channel_metadata);
        
        Py_DECREF(py_channel_metadata);
        
        PyList_SetItem(py_channels, i, py_channel);
        
        i++;
    }

    //printf("got here pre-metadata\n");
    // session metadata
    py_metadata = fill_metadata(sess->time_series_metadata_fps, &sess->time_slice);
    //printf("got here post-metadata\n");
    
    py_password_hints = Py_BuildValue("{s:s,s:s}", "level_1", check_utf8(globals_m12->password_data.level_1_password_hint), "level_2", check_utf8(globals_m12->password_data.level_2_password_hint));
    
    // Create session output structure
    py_info = Py_BuildValue("{s:O,s:O,s:O,s:O}",
                            "metadata", py_metadata,
                            "channels", py_channels,
                            "contigua", py_contigua,
                            "password_hints", py_password_hints);
    Py_DECREF(py_metadata);
    Py_DECREF(py_channels);
    Py_DECREF(py_contigua);
    Py_DECREF(py_password_hints);
    
    return (py_info);
    
    
}


static PyObject *read_MED(PyObject *self, PyObject *args)
{
    //char                    output_statement[1024];
    TERN_m12                                samples_as_singles;
    PyObject                                *sess_capsule_object;
    PyObject                                *password_input_obj;
    PyObject                                *reference_channel_input_obj;
    PyObject                                *samples_as_singles_input_obj;
    PyObject                                *start_time_input_obj, *end_time_input_obj, *start_index_input_obj, *end_index_input_obj;
    PyObject                                *py_channel_records, *py_channel_contigua;
    PyObject                                *py_out_tuple, *py_channel, *py_records, *py_contigua, *py_channels;
    PyArrayObject                           *py_array_out;
    npy_intp                                dims[1];
    si4                                     *numpy_arr_data;
    si1                                     password[PASSWORD_BYTES_m12];
    si1                                     reference_channel[FULL_FILE_NAME_BYTES_m12];
    si8                                     start_time, end_time, start_index, end_index;
    si4                                     i, j, k, m, n;
    si4                                     n_channels, n_segments, seg_offset, n_active_chans;
    si4                                     *seg_samps;
    si8                                     n_seg_samps;
    si1                                     *temp_str_bytes;
    PyObject                                *temp_UTF_str;
    PyObject                                *py_return_object;
    SESSION_m12                             *sess;
    CHANNEL_m12                             *chan;
    SEGMENT_m12                             *seg;
    TIME_SLICE_m12                          local_time_slice, *slice, *sess_slice, *chan_slice;
    TIME_SERIES_METADATA_SECTION_2_m12      *sess_tmd2, *chan_tmd2;
    CMP_PROCESSING_STRUCT_m12               *cps;

    // initialize Numpy
    import_array();
    
    // set defaults for optional arguements
    start_time = UUTC_NO_ENTRY_m12;
    end_time = UUTC_NO_ENTRY_m12;
    start_index = NUMBER_OF_SAMPLES_NO_ENTRY_m12;
    end_index = NUMBER_OF_SAMPLES_NO_ENTRY_m12;
    start_time_input_obj = NULL;
    end_time_input_obj = NULL;
    start_index_input_obj = NULL;
    end_index_input_obj = NULL;
    
    password_input_obj = NULL;
    reference_channel_input_obj = NULL;
    samples_as_singles_input_obj = NULL;
    samples_as_singles = FALSE_m12;
    
    // --- Parse the input ---
    if (!PyArg_ParseTuple(args,"O|OOOO",
                          &sess_capsule_object,
                          &start_time_input_obj,
                          &end_time_input_obj,
                          &start_index_input_obj,
                          &end_index_input_obj)){
        
        PyErr_SetString(PyExc_RuntimeError, "One to 8 inputs required: file_list, [start_time], [end_time], [start_samp_num], [end_samp_num], [password], [reference_channel], [samples_as_singles]\n");
        PyErr_Occurred();
        return NULL;
    }
    
    sess = (SESSION_m12*)PyCapsule_GetPointer(sess_capsule_object, "session");

    if (sess == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Could not get session pointer from capsule\n");
        PyErr_Occurred();
        return NULL;
    }

    slice = &local_time_slice; // this takes care of freeing memory
    G_initialize_time_slice_m12(slice);
    initialize_time_slice(slice, start_index_input_obj, end_index_input_obj, start_time_input_obj, end_time_input_obj);
//    G_show_time_slice_m12(slice);

    G_read_session_m12(sess, slice);


    if (sess == NULL) {
        if (globals_m12->password_data.processed == 0) {
            G_warning_message_m12("\nread_MED():\nCannot read session => no matching input files.\n");
            PyErr_SetString(PyExc_RuntimeError, "\nread_MED():\nCannot read session => no matching input files.\n");
            PyErr_Occurred();
            return NULL;
        } else {
            G_warning_message_m12("\nread_MED():\nCannot read session => Check that the password is correct, and that metadata files exist.\n");
            G_show_password_hints_m12(NULL);
            PyErr_SetString(PyExc_RuntimeError, "\nread_MED():\nCannot read session => Check that the password is correct, and that metadata files exist.\n");
            PyErr_Occurred();
            return NULL;
        }
    }
    // use slice from G_read_session_m12();
    sess_slice = &sess->time_slice;
//    G_show_time_slice_m12(sess_slice);
//    printf("after:");
//    G_show_time_slice_m12(sess_slice);
//    printf("got here b3 slice: %ld %ld\n", sess_slice->start_time, sess_slice->end_time);

    //     ******************************************
    //     ********  Create Python structure  *******
    //     ******************************************

    py_channels = Py_None;
    py_contigua = Py_None;

    n_channels = sess->number_of_time_series_channels;

    for (i = n_active_chans = 0; i < sess->number_of_time_series_channels; ++i) {
        chan = sess->time_series_channels[i];
        if (chan->flags & LH_CHANNEL_ACTIVE_m12)
            ++n_active_chans;
    }


    // Create channel sub-structures
    py_channels = PyList_New(n_active_chans);

    n_segments = sess->time_slice.number_of_segments;

    for (i = j = 0; j < n_channels; ++j) {
        chan = sess->time_series_channels[j];
        if ((chan->flags & LH_CHANNEL_ACTIVE_m12) == 0)
            continue;

        chan_slice = &chan->time_slice;
        chan_tmd2 = &chan->metadata_fps->metadata->time_series_section_2;

        dims[0] = TIME_SLICE_SAMPLE_COUNT_S_m12(chan->time_slice);
        seg_offset = G_get_segment_index_m12(sess->time_slice.start_segment_number);

        if (n_segments == 1) {
            seg = chan->segments[seg_offset];
            cps = seg->time_series_data_fps->parameters.cps;
            seg_samps = cps->decompressed_data;
            py_array_out = (PyArrayObject *) PyArray_SimpleNew(1, dims, NPY_INT);
            numpy_arr_data = (si4 *) PyArray_GETPTR1(py_array_out, 0);
            memcpy(numpy_arr_data, cps->decompressed_data, TIME_SLICE_SAMPLE_COUNT_S_m12(seg->time_slice) * sizeof(si4));

            // Ownership transfer currently causes double free corruption likely caused by caching)
//            // Transfer ownership of the data to Numpy Array
//            py_array_out = (PyArrayObject *) PyArray_SimpleNewFromData(1, dims, NPY_INT, (void *) cps->decompressed_data);
//            PyArray_ENABLEFLAGS((PyArrayObject*) py_array_out, NPY_ARRAY_OWNDATA);
//
//            // Free cache and reset cps for new reads
//            CMP_free_cache_m12(cps);
//            cps->decompressed_ptr = cps->decompressed_data = NULL;
//            cps->parameters.allocated_decompressed_samples = 0;

        } else {
            py_array_out = (PyArrayObject *) PyArray_SimpleNew(1, dims, NPY_INT);
            numpy_arr_data = (si4 *) PyArray_GETPTR1(py_array_out, 0);
            for (m=0,n=seg_offset;m<n_segments; ++m,++n) {
                seg = chan->segments[n];
                cps = seg->time_series_data_fps->parameters.cps;
                seg_samps = cps->decompressed_data;
                n_seg_samps = TIME_SLICE_SAMPLE_COUNT_S_m12(seg->time_slice);
                for (k=0;k<n_seg_samps;++k)
                    numpy_arr_data[k] = seg_samps[k];
                numpy_arr_data += n_seg_samps;
            }
        }


        // Create session metadata output structure
//        sess_tmd2 = &sess->time_series_metadata_fps->metadata->time_series_section_2;
//        if (sess_tmd2->sampling_frequency != FREQUENCY_VARIABLE_m12 && sess_tmd2->sampling_frequency != FREQUENCY_NO_ENTRY_m12) {
//            chan_tmd2 = &sess->time_series_channels[0]->metadata_fps->metadata->time_series_section_2;
//            sess_tmd2->absolute_start_sample_number = chan_tmd2->absolute_start_sample_number;
//            sess_tmd2->number_of_samples = chan_tmd2->number_of_samples;
//        }

        // fill in channel metadata
//        chan_tmd2 = &chan->metadata_fps->metadata->time_series_section_2;
//        chan_tmd2->absolute_start_sample_number = chan_slice->start_sample_number;
//        chan_tmd2->number_of_samples = TIME_SLICE_SAMPLE_COUNT_m12(chan_slice);
//        py_channel_metadata = fill_metadata(chan->metadata_fps, chan_slice);
        // fill in channel contigua
//        py_channel_contigua = build_contigua(chan, chan_slice->start_time, chan_slice->end_time);
//
//        py_channel_records = PyList_New(0);
//        py_channel = Py_BuildValue("{s:O,s:O,s:O}",
////                                    "metadata", py_channel_metadata,
//                                    "data", py_array_out,
//                                    "records", py_channel_records,
//                                    "contigua", py_channel_contigua);
//
////        Py_DECREF(py_channel_metadata);
//        Py_DECREF(py_array_out);
//        Py_DECREF(py_channel_records);
//        Py_DECREF(py_channel_contigua);
        PyList_SetItem(py_channels, i, py_array_out);
        ++i;

    }

//    printf("got here pre-metadata\n");
    // session metadata
//    py_metadata = fill_metadata(sess->time_series_metadata_fps, sess_slice);
//    printf("got here post-metadata\n");

    // session records
    // py_records = fill_session_records(sess, NULL);

//    printf("go here post-records\n");

    // Create session contiguous samples output structure
//    printf("Builiding session contigua\n");
//    if (n_channels > 0) {
//
//        PyObject                        *py_contiguon;
//        PyObject                        *first_chan;
//        PyObject                        *first_chan_contigua;
//        PyObject                        *py_contiguon_read;
//        si8                             current_start_time, current_start_index, current_end_time, current_end_index;
//
//        first_chan = PyList_GetItem(py_channels, 0);
//        first_chan_contigua = PyObject_GetItem(first_chan, Py_BuildValue("s", "contigua"));
//        int n_contigua = PyList_GET_SIZE(first_chan_contigua);
//
//        py_contigua = PyList_New(n_contigua);
//
//        for (i=0; i< n_contigua; ++i) {
//            py_contiguon_read = PyList_GetItem(first_chan_contigua, i);
//            sess_tmd2 = &sess->time_series_metadata_fps->metadata->time_series_section_2;
//            if (sess_tmd2->sampling_frequency == FREQUENCY_VARIABLE_m12) {
//                current_start_index = -1;
//                current_end_index = -1;
//            } else {
//                current_start_index = PyLong_AsLongLong(PyObject_GetItem(py_contiguon_read, Py_BuildValue("s", "start_index")));
//                current_end_index = PyLong_AsLongLong(PyObject_GetItem(py_contiguon_read, Py_BuildValue("s", "end_index")));
//            }
//            current_start_time = PyLong_AsLongLong(PyObject_GetItem(py_contiguon_read, Py_BuildValue("s", "start_time")));
//            current_end_time = PyLong_AsLongLong(PyObject_GetItem(py_contiguon_read, Py_BuildValue("s", "end_time")));
//            py_contiguon = Py_BuildValue("{s:L,s:L,s:L,s:L}",
//                                         "start_index", current_start_index,
//                                         "end_index", current_end_index,
//                                         "start_time", current_start_time,
//                                         "end_time", current_end_time);
//
//            PyList_SetItem(py_contigua, i, py_contiguon);
//        }
//    } else {
//        // No channels ... so create empty list, and we are done.
//        py_contigua = PyList_New(n_channels);
//    }
//
//    py_out_tuple = PyTuple_New(2);
//    PyTuple_SetItem(py_out_tuple, 0, py_channels);
//    PyTuple_SetItem(py_out_tuple, 1, py_contigua);

    // Create session output structure
//    py_session = Py_BuildValue("{s:O,s:O,s:O}",
//                               "channels", py_channels,
//                               "contigua", py_contigua);

    // clean up
//    Py_DECREF(py_channels);
//    Py_DECREF(py_records);
//    Py_DECREF(py_contigua);

    return py_channels;
}


PyObject*   build_contigua(CHANNEL_m12 *chan, si8 start_time, si8 end_time)
{
    si4                n_segs;
    si8                             i, j, k, n_inds, start_idx, end_idx, n_disconts, n_contigs, abs_offset;
    si8                             total_samps_in_segs, local_end_time, local_end_sample;
    si4                             seg_offset;
    SEGMENT_m12                     *seg;
    TIME_SERIES_INDEX_m12           *tsi;
    PyObject                        *py_contigua_list;
    PyObject                        *py_contiguon;
    si8                             current_start_time, current_start_index, current_end_time, current_end_index;
    
    n_segs = chan->time_slice.number_of_segments;
    //seg_offset = G_get_segment_offset_m12((LEVEL_HEADER_m12 *) chan);
    seg_offset = G_get_segment_index_m12(chan->time_slice.start_segment_number);
    for (n_disconts = i = 0, k=seg_offset; i < n_segs; ++i, ++k) {
        seg = chan->segments[k];
        abs_offset = seg->metadata_fps->metadata->time_series_section_2.absolute_start_sample_number;
        start_idx = seg->time_slice.start_sample_number - abs_offset;
        end_idx = seg->time_slice.end_sample_number - abs_offset;
        tsi = seg->time_series_indices_fps->time_series_indices;
        n_inds = seg->time_series_indices_fps->universal_header->number_of_entries;
        for (j = 0; j < n_inds; ++j)
            if (tsi[j].start_sample_number > start_idx)
                break;
        for (; j < n_inds; ++j) {
            if (tsi[j].start_sample_number > end_idx)
                break;
            if (tsi[j].file_offset < 0)
                ++n_disconts;
        }
    }
    n_contigs = n_disconts + 1;
    
    py_contigua_list = PyList_New(n_contigs);
    
    current_start_index = 1;
    current_start_time = start_time;
    
    n_disconts = 0;
    total_samps_in_segs = 0;
    for (i = 0, k=seg_offset; i < n_segs; ++i, ++k) {
        seg = chan->segments[k];
        abs_offset = seg->metadata_fps->metadata->time_series_section_2.absolute_start_sample_number;
        start_idx = seg->time_slice.start_sample_number - abs_offset;
        end_idx = seg->time_slice.end_sample_number - abs_offset;
        tsi = seg->time_series_indices_fps->time_series_indices;
        n_inds = seg->time_series_indices_fps->universal_header->number_of_entries;
        for (j = 0; j < n_inds; ++j)
            if (tsi[j].start_sample_number > start_idx)
                break;
        for (; j < n_inds; ++j) {
            if (tsi[j].start_sample_number > end_idx)
                break;
            if (tsi[j].file_offset < 0) {
                current_end_index = tsi[j].start_sample_number + total_samps_in_segs;
                local_end_sample = tsi[j].start_sample_number - 1;
                local_end_time = G_uutc_for_sample_number_m12((LEVEL_HEADER_m12 *) seg, local_end_sample, FIND_END_m12);
                current_end_time = local_end_time;
                
                py_contiguon = Py_BuildValue("{s:L,s:L,s:L,s:L}",
                                             "start_index", current_start_index,
                                             "end_index", current_end_index,
                                             "start_time", current_start_time,
                                             "end_time", current_end_time);
                
                PyList_SetItem(py_contigua_list, n_disconts, py_contiguon);
                
                ++n_disconts;
                
                current_start_index = tsi[j].start_sample_number + total_samps_in_segs + 1;
                current_start_time = tsi[j].start_time;
            }
        }
        total_samps_in_segs += ((end_idx - start_idx) + 1);
    }
    
    current_end_index = total_samps_in_segs;
    current_end_time = end_time;
    
    py_contiguon = Py_BuildValue("{s:L,s:L,s:L,s:L}",
                                 "start_index", current_start_index,
                                 "end_index", current_end_index,
                                 "start_time", current_start_time,
                                 "end_time", current_end_time);
    
    PyList_SetItem(py_contigua_list, n_disconts, py_contiguon);
    
    return(py_contigua_list);
}


PyObject*    fill_metadata(FILE_PROCESSING_STRUCT_m12 *metadata_fps, TIME_SLICE_m12 *slice)
{
    extern GLOBALS_m12            *globals_m12;
    si1                                     time_str_start_time[TIME_STRING_BYTES_m12], time_str_end_time[TIME_STRING_BYTES_m12],
    time_str_session_start_time[TIME_STRING_BYTES_m12], time_str_session_end_time[TIME_STRING_BYTES_m12];
    si1                                     path[FULL_FILE_NAME_BYTES_m12];
    UNIVERSAL_HEADER_m12                    *uh;
    TIME_SERIES_METADATA_SECTION_2_m12      *tmd2;
    METADATA_SECTION_3_m12                  *md3;
    si8                                     tmp_mxa_start, tmp_mxa_end;
    sf8                                         tmp_mxa_samp_freq;
    PyObject                                *py_metadata;
    

    uh = metadata_fps->universal_header;
    tmd2 = &metadata_fps->metadata->time_series_section_2;
    md3 = &metadata_fps->metadata->section_3;
    //printf("%s\n", metadata_fps->full_file_name);
    G_extract_path_parts_m12(metadata_fps->full_file_name, path, NULL, NULL);
    // start time string
    if (globals_m12->RTO_known == TRUE_m12)
        STR_time_string_m12(slice->start_time, time_str_start_time, TRUE_m12, FALSE_m12, FALSE_m12);
    else
        STR_time_string_m12(slice->start_time, time_str_start_time, TRUE_m12, TRUE_m12, FALSE_m12);
    
    // end time string
    if (globals_m12->RTO_known == TRUE_m12)
        STR_time_string_m12(slice->end_time, time_str_end_time, TRUE_m12, FALSE_m12, FALSE_m12);
    else
        STR_time_string_m12(slice->end_time, time_str_end_time, TRUE_m12, TRUE_m12, FALSE_m12);
    
    // session start time string
    if (globals_m12->RTO_known == TRUE_m12)
        STR_time_string_m12(globals_m12->session_start_time, time_str_session_start_time, TRUE_m12, FALSE_m12, FALSE_m12);
    else
        STR_time_string_m12(globals_m12->session_start_time, time_str_session_start_time, TRUE_m12, TRUE_m12, FALSE_m12);
    
    // session end time string
    if (globals_m12->RTO_known == TRUE_m12)
        STR_time_string_m12(globals_m12->session_end_time, time_str_session_end_time, TRUE_m12, FALSE_m12, FALSE_m12);
    else
        STR_time_string_m12(globals_m12->session_end_time, time_str_session_end_time, TRUE_m12, TRUE_m12, FALSE_m12);

    // absolute start sample number
    if (tmd2->sampling_frequency == FREQUENCY_VARIABLE_m12 || tmd2->sampling_frequency == FREQUENCY_NO_ENTRY_m12)
        tmp_mxa_start = -1;  // convert to one-based indexing
    else
        tmp_mxa_start = slice->start_sample_number;
    
    // absolute end sample number
    if (tmd2->sampling_frequency == FREQUENCY_VARIABLE_m12 || tmd2->sampling_frequency == FREQUENCY_NO_ENTRY_m12)
        tmp_mxa_end = -1;
    else
        tmp_mxa_end = slice->end_sample_number + 1;  // end is exclusive in python, but inclusive in slice
    
    // sampling frequency
    if (tmd2->sampling_frequency == FREQUENCY_VARIABLE_m12 || tmd2->sampling_frequency == FREQUENCY_NO_ENTRY_m12)
        tmp_mxa_samp_freq = -1;
    else
        tmp_mxa_samp_freq = tmd2->sampling_frequency;

    if (metadata_fps->parameters.password_data->access_level >= metadata_fps->metadata->section_1.section_3_encryption_level) {
        py_metadata = Py_BuildValue("{s:s,s:L,s:L,s:s,s:s,s:L,s:L,s:s,s:s,s:L,s:L,s:s,s:s,s:s,s:K,s:K,s:s,s:s,s:s,s:i,s:s,s:f,s:d,s:d,s:d,s:d,s:d,s:s,s:d,s:s,s:L,s:i,s:s,s:s,s:s,s:s,s:L,s:L,s:s,s:s,s:s,s:s,s:s,s:s,s:s,s:s}",
                                    
                                    "path", check_utf8(path),
                                    "start_time", slice->start_time,
                                    "end_time", slice->end_time,
                                    "start_time_string", check_utf8(time_str_start_time),
                                    "end_time_string", check_utf8(time_str_end_time),
                                    "session_start_time", globals_m12->session_start_time,
                                    "session_end_time", globals_m12->session_end_time,
                                    "session_start_time_string", check_utf8(time_str_session_start_time),
                                    "session_end_time_string", check_utf8(time_str_session_end_time),
                                    "absolute_start_sample_number", tmp_mxa_start,
                                    "absolute_end_sample_number", tmp_mxa_end,
                                    "session_name", check_utf8(uh->session_name),
                                    "channel_name", check_utf8(uh->channel_name),
                                    "anonymized_subject_ID", check_utf8(uh->anonymized_subject_ID),
                                    "session_UID", uh->session_UID,
                                    "channel_UID", uh->channel_UID,
                                    "session_description", check_utf8(tmd2->session_description),
                                    "channel_description", check_utf8(tmd2->channel_description),
                                    "equipment_description", check_utf8(tmd2->equipment_description),
                                    "acquisition_channel_number", tmd2->acquisition_channel_number,
                                    "reference_description", check_utf8(tmd2->reference_description),
                                    "sampling_frequency", tmp_mxa_samp_freq,
                                    "low_frequency_filter_setting", tmd2->low_frequency_filter_setting,
                                    "high_frequency_filter_setting", tmd2->high_frequency_filter_setting,
                                    "notch_filter_frequency_setting", tmd2->notch_filter_frequency_setting,
                                    "AC_line_frequency", tmd2->AC_line_frequency,
                                    "amplitude_units_conversion_factor", tmd2->amplitude_units_conversion_factor,
                                    "amplitude_units_description", check_utf8(tmd2->amplitude_units_description),
                                    "time_base_units_conversion_factor", tmd2->time_base_units_conversion_factor,
                                    "time_base_units_description", check_utf8(tmd2->time_base_units_description),
                                    "recording_time_offset",md3->recording_time_offset,
                                    "standard_UTC_offset", md3->standard_UTC_offset,
                                    "standard_timezone_string", check_utf8(md3->standard_timezone_string),
                                    "standard_timezone_acronym", check_utf8(md3->standard_timezone_acronym),
                                    "daylight_timezone_string", check_utf8(md3->daylight_timezone_string),
                                    "daylight_timezone_acronym", check_utf8(md3->daylight_timezone_acronym),
                                    "daylight_time_start_code", md3->daylight_time_start_code.value,
                                    "daylight_time_end_code", md3->daylight_time_end_code.value,
                                    "subject_name_1", check_utf8(md3->subject_name_1),
                                    "subject_name_2", check_utf8(md3->subject_name_2),
                                    "subject_name_3", check_utf8(md3->subject_name_3),
                                    "subject_ID", check_utf8(md3->subject_ID),
                                    "recording_country", check_utf8(md3->recording_country),
                                    "recording_territory", check_utf8(md3->recording_territory),
                                    "recording_locality", check_utf8(md3->recording_locality),
                                    "recording_institution", check_utf8(md3->recording_institution)
                                    );
    } else {
        py_metadata = Py_BuildValue("{s:s,s:L,s:L,s:s,s:s,s:L,s:L,s:s,s:s,s:L,s:L,s:s,s:s,s:s,s:K,s:K,s:s,s:s,s:s,s:i,s:s,s:f,s:d,s:d,s:d,s:d,s:d,s:s,s:d,s:s,s:L,s:i,s:s,s:s,s:s,s:s,s:s,s:s,s:s,s:s,s:s,s:s,s:s,s:s,s:s,s:s}",
                                    "path", check_utf8(path),
                                    "start_time", slice->start_time,
                                    "end_time", slice->end_time,
                                    "start_time_string", check_utf8(time_str_start_time),
                                    "end_time_string", check_utf8(time_str_end_time),
                                    "session_start_time", globals_m12->session_start_time,
                                    "session_end_time", globals_m12->session_end_time,
                                    "session_start_time_string", check_utf8(time_str_session_start_time),
                                    "session_end_time_string", check_utf8(time_str_session_end_time),
                                    "absolute_start_sample_number", tmp_mxa_start,
                                    "absolute_end_sample_number", tmp_mxa_end,
                                    "session_name", check_utf8(uh->session_name),
                                    "channel_name", check_utf8(uh->channel_name),
                                    "anonymized_subject_ID", check_utf8(uh->anonymized_subject_ID),
                                    "session_UID", uh->session_UID,
                                    "channel_UID", uh->channel_UID,
                                    "session_description", check_utf8(tmd2->session_description),
                                    "channel_description", check_utf8(tmd2->channel_description),
                                    "equipment_description", check_utf8(tmd2->equipment_description),
                                    "acquisition_channel_number", tmd2->acquisition_channel_number,
                                    "reference_description", check_utf8(tmd2->reference_description),
                                    "sampling_frequency", tmp_mxa_samp_freq,
                                    "low_frequency_filter_setting", tmd2->low_frequency_filter_setting,
                                    "high_frequency_filter_setting", tmd2->high_frequency_filter_setting,
                                    "notch_filter_frequency_setting", tmd2->notch_filter_frequency_setting,
                                    "AC_line_frequency", tmd2->AC_line_frequency,
                                    "amplitude_units_conversion_factor", tmd2->amplitude_units_conversion_factor,
                                    "amplitude_units_description", check_utf8(tmd2->amplitude_units_description),
                                    "time_base_units_conversion_factor", tmd2->time_base_units_conversion_factor,
                                    "time_base_units_description", check_utf8(tmd2->time_base_units_description),
                                    "recording_time_offset", 0,
                                    "standard_UTC_offset", 0,
                                    "standard_timezone_string", "offset Coordinated Universal Time",
                                    "standard_timezone_acronym", "oUTC",
                                    "daylight_timezone_string", "no access",
                                    "daylight_timezone_acronym", "no access",
                                    "daylight_time_start_code", "no access",
                                    "daylight_time_end_code", "no access",
                                    "subject_name_1", "no access",
                                    "subject_name_2", "no access",
                                    "subject_name_3", "no access",
                                    "subject_ID", "no access",
                                    "recording_country", "no access",
                                    "recording_territory", "no access",
                                    "recording_locality", "no access",
                                    "recording_institution", "no access"
                                    );
    }
    return (py_metadata);
}


PyObject*    fill_session_records(SESSION_m12 *sess,  DATA_MATRIX_m12 *dm)
{
    si4                     n_segs, seg_idx;
    si8                     i, j, k, n_items, tot_recs, n_recs;
    ui1                     *rd;
    RECORD_HEADER_m12       **rec_ptrs, *rh;
    PyObject                *py_record_list;
    PyObject                *temp_record;
    FILE_PROCESSING_STRUCT_m12    *rd_fps;
    

    n_segs = sess->time_slice.number_of_segments;
    
    // set up sorted records array
    tot_recs = 0;
    if (sess->record_data_fps != NULL && sess->record_indices_fps != NULL)
        tot_recs = sess->record_data_fps->number_of_items;
    if (sess->segmented_sess_recs != NULL) {
        seg_idx = G_get_segment_index_m12(sess->time_slice.start_segment_number);
        for (i = 0, j = seg_idx; i < n_segs; ++i, ++j) {
            rd_fps = sess->segmented_sess_recs->record_data_fps[j];
                if (rd_fps != NULL)
                    tot_recs += rd_fps->number_of_items;
        }
    }
    if (tot_recs == 0)
        return PyList_New(0);

    rec_ptrs = (RECORD_HEADER_m12 **) malloc((size_t) tot_recs * sizeof(RECORD_HEADER_m12 *));
    n_recs = 0;
    if (sess->record_data_fps != NULL) {
        n_items = sess->record_data_fps->number_of_items;
        rd = sess->record_data_fps->record_data;
        for (i = 0; i < n_items; ++i) {
            rh = (RECORD_HEADER_m12 *) rd;
            switch (rh->type_code) {
                    // excluded types
                case REC_Term_TYPE_CODE_m12:
                case REC_SyLg_TYPE_CODE_m12:
                    break;
                default:  // include all other record types
                    rec_ptrs[n_recs++] = rh;
                    break;
            }
            rd += rh->total_record_bytes;
        }
    }
    if (sess->segmented_sess_recs != NULL) {
        for (i = 0, j = seg_idx; i < n_segs; ++i, ++j) {
            rd_fps = sess->segmented_sess_recs->record_data_fps[j];
                if (rd_fps == NULL)
                    continue;
            n_items = sess->segmented_sess_recs->record_data_fps[j]->number_of_items;
            rd = sess->segmented_sess_recs->record_data_fps[j]->record_data;
            for (k = 0; k < n_items; ++k) {
                rh = (RECORD_HEADER_m12 *) rd;
                switch (rh->type_code) {
                        // excluded tyoes
                    case REC_Term_TYPE_CODE_m12:
                    case REC_SyLg_TYPE_CODE_m12:
                    // case REC_SyLg_TYPE_CODE_m12:  // shouldn't be in segmented session records
                        break;
                    default:  // include all other record types
                        rec_ptrs[n_recs++] = rh;
                        break;
                }
                rd += rh->total_record_bytes;
            }
        }
    }
    if (n_recs == 0) {
        if (rec_ptrs != NULL)
            free(rec_ptrs);
        return PyList_New(0);
    }
    qsort((void *) rec_ptrs, n_recs, sizeof(RECORD_HEADER_m12 *), rec_compare);
    
    py_record_list = PyList_New(n_recs);

    // create python records
    for (i = 0; i < n_recs; ++i) {
        if (dm != NULL)
            temp_record = fill_record_matrix(rec_ptrs[i], dm);
        else
            temp_record = fill_record(rec_ptrs[i]);
        PyList_SetItem(py_record_list, i, temp_record);
    }

    // clean up
    if (rec_ptrs != NULL)
        free(rec_ptrs);

    return py_record_list;
}

PyObject    *fill_record(RECORD_HEADER_m12 *rh)
{
    extern GLOBALS_m12    *globals_m12;
    TERN_m12        relative_days;
    si1                     ver_str[8], *enc_str, enc_level, time_str[TIME_STRING_BYTES_m12], time_str_end[TIME_STRING_BYTES_m12];
    REC_NlxP_v10_m12        *NlxP;
    REC_CSti_v10_m12        *CSti;
    REC_ESti_v10_m12        *ESti;
    PyObject                *py_record;
    REC_Sgmt_v10_m12        *Sgmt;
    int valid_text;
    si1   *text;
    sf8                     ver;
    
    // start time string
    STR_time_string_m12(rh->start_time, time_str, TRUE_m12, FALSE_m12, FALSE_m12);

    // version
    ver = (sf8) rh->version_major + ((sf8) rh->version_minor / (sf8) 1000.0);
    sprintf(ver_str, "%0.3lf", ver);
    
    // encryption
    enc_level = rh->encryption_level;
    switch (enc_level) {
        case NO_ENCRYPTION_m12:
            enc_str = "none";
            break;
        case LEVEL_1_ENCRYPTION_m12:
            enc_str = "level 1, encrypted";
            break;
        case LEVEL_2_ENCRYPTION_m12:
            enc_str = "level 2, encrypted";
            break;
        case LEVEL_1_ENCRYPTION_DECRYPTED_m12:
            enc_str = "level 1, decrypted";
            break;
        case LEVEL_2_ENCRYPTION_DECRYPTED_m12:
            enc_str = "level 2, decrypted";
            break;
        default:
            enc_str = "<unrecognized level>";
            enc_level = LEVEL_1_ENCRYPTION_m12;  // set to any encrypted level
            break;
    }
    

    if (globals_m12->RTO_known == TRUE_m12)
        relative_days = FALSE_m12;
    else
        relative_days = TRUE_m12;
    
    enc_level = rh->encryption_level;
    
    // create PyObject
    switch (rh->type_code) {
        case REC_Note_TYPE_CODE_m12:
            py_record = Py_BuildValue("{s:L,s:s,s:s,s:I,s:s,s:I,s:s,s:s}",
                                      "start_time", rh->start_time,
                                      "start_time_string", check_utf8(time_str),
                                      "type_string", check_utf8(rh->type_string),
                                      "type_code", rh->type_code,
                                      "version_string", check_utf8(ver_str),
                                      "encryption", enc_level,
                                      "encryption_string", check_utf8(enc_str),
                                      "text", enc_level <= 0 ? check_utf8((si1 *) rh + RECORD_HEADER_BYTES_m12) : "<no access>");
            break;
        case REC_NlxP_TYPE_CODE_m12:
            NlxP = (REC_NlxP_v10_m12 *) ((ui1 *) rh + RECORD_HEADER_BYTES_m12);
            if (enc_level <= 0) {
                py_record = Py_BuildValue("{s:L,s:s,s:s,s:I,s:s,s:I,s:s,s:I,s:I}",
                                          "start_time", rh->start_time,
                                          "start_time_string", check_utf8(time_str),
                                          "type_string", check_utf8(rh->type_string),
                                          "type_code", rh->type_code,
                                          "version_string", check_utf8(ver_str),
                                          "encryption", enc_level,
                                          "encryption_string", check_utf8(enc_str),
                                          "subport", NlxP->subport,
                                          "value", NlxP->value);
            } else {
                py_record = Py_BuildValue("{s:L,s:s,s:s,s:I,s:s,s:I,s:s,s:s,s:s}",
                                          "start_time", rh->start_time,
                                          "start_time_string", check_utf8(time_str),
                                          "type_string", check_utf8(rh->type_string),
                                          "type_code", rh->type_code,
                                          "version_string", check_utf8(ver_str),
                                          "encryption", enc_level,
                                          "encryption_string", check_utf8(enc_str),
                                          "subport", "<no access>",
                                          "value", "<no access>");
            }
            break;
        case REC_ESti_TYPE_CODE_m12:
            ESti = (REC_ESti_v10_m12 *) ((ui1 *) rh + RECORD_HEADER_BYTES_m12);
            if (enc_level <= 0) {
                py_record = Py_BuildValue("{s:L,s:s,s:s,s:I,s:s,s:I,s:s,s:d,s:d,s:L,s:I,s:I,s:s,s:s,s:s}",
                                          "start_time", rh->start_time,
                                          "start_time_string", check_utf8(time_str),
                                          "type_string", check_utf8(rh->type_string),
                                          "type_code", rh->type_code,
                                          "version_string", check_utf8(ver_str),
                                          "encryption", enc_level,
                                          "encryption_string", check_utf8(enc_str),
                                          "amplitude", ESti->amplitude,
                                          "frequency", ESti->frequency,
                                          "pulse_width", ESti->pulse_width,
                                          "amp_unit_code", ESti->amp_unit_code,
                                          "mode_code", ESti->mode_code,
                                          "waveform", check_utf8(ESti->waveform),
                                          "anode", check_utf8(ESti->anode),
                                          "cathode", check_utf8(ESti->cathode));
            } else {
                py_record = Py_BuildValue("{s:L,s:s,s:s,s:I,s:s,s:I,s:s,s:s,s:s,s:s,s:s,s:s,s:s,s:s,s:s}",
                                          "start_time", rh->start_time,
                                          "start_time_string", check_utf8(time_str),
                                          "type_string", check_utf8(rh->type_string),
                                          "type_code", rh->type_code,
                                          "version_string", check_utf8(ver_str),
                                          "encryption", enc_level,
                                          "encryption_string", check_utf8(enc_str),
                                          "amplitude", "<no access>",
                                          "frequency", "<no access>",
                                          "pulse_width", "<no access>",
                                          "amp_unit_code", "<no access>",
                                          "mode_code", "<no access>",
                                          "waveform", "<no access>",
                                          "anode", "<no access>",
                                          "cathode", "<no access>");
            }
            break;
        case REC_CSti_TYPE_CODE_m12:
            CSti = (REC_CSti_v10_m12 *) ((ui1 *) rh + RECORD_HEADER_BYTES_m12);
            if (enc_level <= 0) {
                py_record = Py_BuildValue("{s:L,s:s,s:s,s:I,s:s,s:I,s:s,s:I,s:s,s:s,s:s}",
                                          "start_time", rh->start_time,
                                          "start_time_string", check_utf8(time_str),
                                          "type_string", check_utf8(rh->type_string),
                                          "type_code", rh->type_code,
                                          "version_string", check_utf8(ver_str),
                                          "encryption", enc_level,
                                          "encryption_string", check_utf8(enc_str),
                                          "stimulus_duration", CSti->stimulus_duration,
                                          "task_type", check_utf8(CSti->task_type),
                                          "stimulus_type", check_utf8(CSti->stimulus_type),
                                          "patient_response", check_utf8(CSti->patient_response));
            } else {
                py_record = Py_BuildValue("{s:L,s:s,s:s,s:I,s:s,s:I,s:s,s:s,s:s,s:s,s:s}",
                                          "start_time", rh->start_time,
                                          "start_time_string", check_utf8(time_str),
                                          "type_string", check_utf8(rh->type_string),
                                          "type_code", rh->type_code,
                                          "version_string", check_utf8(ver_str),
                                          "encryption", enc_level,
                                          "encryption_string", check_utf8(enc_str),
                                          "stimulus_duration", "<no access>",
                                          "task_type", "<no access>",
                                          "stimulus_type", "<no access>",
                                          "patient_response", "<no access>");
            }
            break;
        case REC_Sgmt_TYPE_CODE_m12:
            Sgmt = (REC_Sgmt_v10_m12 *) ((ui1 *) rh + RECORD_HEADER_BYTES_m12);
            if (enc_level <= 0) {
                STR_time_string_m12(Sgmt->end_time, time_str_end, TRUE_m12, relative_days, FALSE_m12);
                
                valid_text = 0;;
                if (rh->total_record_bytes > (RECORD_HEADER_BYTES_m12 + REC_Sgmt_v10_BYTES_m12)) {
                    text = (si1 *) rh + RECORD_HEADER_BYTES_m12 + REC_Sgmt_v10_BYTES_m12;
                    if (*text)
                        valid_text = 1;
                }
                if (Sgmt->acquisition_channel_number != REC_Sgmt_v10_ACQUISITION_CHANNEL_NUMBER_ALL_CHANNELS_m12) {
                    if (Sgmt->sampling_frequency != REC_Sgmt_v10_SAMPLING_FREQUENCY_VARIABLE_m12) {
                        py_record = Py_BuildValue("{s:L,s:s,s:s,s:I,s:s,s:I,s:s,s:L,s:s,s:L,s:L,s:I,s:K,s:I,s:d,s:s}",
                                                  "start_time", rh->start_time,
                                                  "start_time_string", check_utf8(time_str),
                                                  "type_string", check_utf8(rh->type_string),
                                                  "type_code", rh->type_code,
                                                  "version_string", check_utf8(ver_str),
                                                  "encryption", enc_level,
                                                  "encryption_string", check_utf8(enc_str),
                                                  "end_time", Sgmt->end_time,
                                                  "end_time_string", check_utf8(time_str_end),
                                                  "start_sample_number", Sgmt->start_sample_number,
                                                  "end_sample_number", Sgmt->end_sample_number,
                                                  "segment_number", Sgmt->segment_number,
                                                  "segment_UID", Sgmt->segment_UID,
                                                  "acquistion_channel_number", Sgmt->acquisition_channel_number,
                                                  "sampling_frequency", Sgmt->sampling_frequency,
                                                  "description", valid_text == 1 ? check_utf8(text) : "<no description>");
                    } else {
                        py_record = Py_BuildValue("{s:L,s:s,s:s,s:I,s:s,s:I,s:s,s:L,s:s,s:L,s:L,s:I,s:K,s:I,s:s,s:s}",
                                                  "start_time", rh->start_time,
                                                  "start_time_string", check_utf8(time_str),
                                                  "type_string", check_utf8(rh->type_string),
                                                  "type_code", rh->type_code,
                                                  "version_string", check_utf8(ver_str),
                                                  "encryption", enc_level,
                                                  "encryption_string", check_utf8(enc_str),
                                                  "end_time", Sgmt->end_time,
                                                  "end_time_string", check_utf8(time_str_end),
                                                  "start_sample_number", Sgmt->start_sample_number,
                                                  "end_sample_number", Sgmt->end_sample_number,
                                                  "segment_number", Sgmt->segment_number,
                                                  "segment_UID", Sgmt->segment_UID,
                                                  "acquistion_channel_number", Sgmt->acquisition_channel_number ,
                                                  "sampling_frequency", "variable",
                                                  "description", valid_text == 1 ? check_utf8(text) : "<no description>");
                    }
                } else {
                    if (Sgmt->sampling_frequency != REC_Sgmt_v10_SAMPLING_FREQUENCY_VARIABLE_m12) {
                        py_record = Py_BuildValue("{s:L,s:s,s:s,s:I,s:s,s:I,s:s,s:L,s:s,s:L,s:L,s:I,s:K,s:s,s:d,s:s}",
                                                  "start_time", rh->start_time,
                                                  "start_time_string", check_utf8(time_str),
                                                  "type_string", check_utf8(rh->type_string),
                                                  "type_code", rh->type_code,
                                                  "version_string", check_utf8(ver_str),
                                                  "encryption", enc_level,
                                                  "encryption_string", check_utf8(enc_str),
                                                  "end_time", Sgmt->end_time,
                                                  "end_time_string", check_utf8(time_str_end),
                                                  "start_sample_number", Sgmt->start_sample_number,
                                                  "end_sample_number", Sgmt->end_sample_number,
                                                  "segment_number", Sgmt->segment_number,
                                                  "segment_UID", Sgmt->segment_UID,
                                                  "acquistion_channel_number", "all channels",
                                                  "sampling_frequency", Sgmt->sampling_frequency,
                                                  "description", valid_text == 1 ? check_utf8(text) : "<no description>");
                    } else {
                        py_record = Py_BuildValue("{s:L,s:s,s:s,s:I,s:s,s:I,s:s,s:L,s:s,s:L,s:L,s:I,s:K,s:s,s:s,s:s}",
                                                  "start_time", rh->start_time,
                                                  "start_time_string", check_utf8(time_str),
                                                  "type_string", check_utf8(rh->type_string),
                                                  "type_code", rh->type_code,
                                                  "version_string", check_utf8(ver_str),
                                                  "encryption", enc_level,
                                                  "encryption_string", check_utf8(enc_str),
                                                  "end_time", Sgmt->end_time,
                                                  "end_time_string", check_utf8(time_str_end),
                                                  "start_sample_number", Sgmt->start_sample_number,
                                                  "end_sample_number", Sgmt->end_sample_number,
                                                  "segment_number", Sgmt->segment_number,
                                                  "segment_UID", Sgmt->segment_UID,
                                                  "acquistion_channel_number", "all channels",
                                                  "sampling_frequency", "variable",
                                                  "description", valid_text == 1 ? check_utf8(text) : "<no description>");
                    }
                }
            }
            else {
                py_record = Py_BuildValue("{s:L,s:s,s:s,s:I,s:s,s:I,s:s,s:s,s:s,s:s,s:s,s:s,s:s,s:s,s:s,s:s}",
                                          "start_time", rh->start_time,
                                          "start_time_string", check_utf8(time_str),
                                          "type_string", check_utf8(rh->type_string),
                                          "type_code", rh->type_code,
                                          "version_string", check_utf8(ver_str),
                                          "encryption", enc_level,
                                          "encryption_string", check_utf8(enc_str),
                                          "end_time", "<no access>",
                                          "end_time_string", "<no access>",
                                          "start_sample_number", "<no access>",
                                          "end_sample_number", "<no access>",
                                          "segment_number", "<no access>",
                                          "segment_UID", "<no access>",
                                          "acquistion_channel_number", "<no access>",
                                          "sampling_frequency", "<no access>",
                                          "description", "<no access>");
                
            }
            
            break;
        default:
            py_record = Py_BuildValue("{s:L,s:s,s:s,s:I,s:s,s:I,s:s,s:s}",
                                      "start_time", rh->start_time,
                                      "start_time_string", check_utf8(time_str),
                                      "type_string", check_utf8(rh->type_string),
                                      "type_code", rh->type_code,
                                      "version_string", check_utf8(ver_str),
                                      "encryption", enc_level,
                                      "encryption_string", check_utf8(enc_str),
                                      "comment", enc_level <= 0 ? " unknown record type" : "<no access>");
            break;
    }

    return py_record;
    
    
}


PyObject    *fill_record_matrix(RECORD_HEADER_m12 *rh, DATA_MATRIX_m12 *dm)
{
    extern GLOBALS_m12    *globals_m12;
    TERN_m12        relative_days;
    si1                     ver_str[8], *enc_str, enc_level, time_str[TIME_STRING_BYTES_m12], time_str_end[TIME_STRING_BYTES_m12];
    REC_NlxP_v10_m12        *NlxP;
    REC_CSti_v10_m12        *CSti;
    REC_ESti_v10_m12        *ESti;
    PyObject                *py_record;
    REC_Sgmt_v10_m12        *Sgmt;
    CONTIGUON_m12        *contigua;
    si4            i;
    si8            offset_samps, start_idx;
    sf8                     ver, offset_secs;
    int valid_text;
    si1   *text;
    
    // start time string
    STR_time_string_m12(rh->start_time, time_str, TRUE_m12, FALSE_m12, FALSE_m12);
    
    // version
    ver = (sf8) rh->version_major + ((sf8) rh->version_minor / (sf8) 1000.0);
    sprintf(ver_str, "%0.3lf", ver);
    // encryption
    enc_level = rh->encryption_level;
    switch (enc_level) {
        case NO_ENCRYPTION_m12:
            enc_str = "none";
            break;
        case LEVEL_1_ENCRYPTION_m12:
            enc_str = "level 1, encrypted";
            break;
        case LEVEL_2_ENCRYPTION_m12:
            enc_str = "level 2, encrypted";
            break;
        case LEVEL_1_ENCRYPTION_DECRYPTED_m12:
            enc_str = "level 1, decrypted";
            break;
        case LEVEL_2_ENCRYPTION_DECRYPTED_m12:
            enc_str = "level 2, decrypted";
            break;
        default:
            enc_str = "<unrecognized level>";
            enc_level = LEVEL_1_ENCRYPTION_m12;  // set to any encrypted level
            break;
    }
    
    
    if (globals_m12->RTO_known == TRUE_m12)
        relative_days = FALSE_m12;
    else
        relative_days = TRUE_m12;
    
    enc_level = rh->encryption_level;
    
    // start index (in matrix reference frame)
    contigua = dm->contigua;
    for (i = 0; i < dm->number_of_contigua; ++i)
        if (rh->start_time <= contigua[i].end_time)
            break;
    offset_secs = (sf8) (rh->start_time - contigua[i].start_time) / (sf8) 1e6;
    offset_samps = (si8) round(offset_secs * dm->sampling_frequency);
    start_idx = contigua[i].start_sample_number + offset_samps;
    // create PyObject
    switch (rh->type_code) {
        case REC_Note_TYPE_CODE_m12:
            py_record = Py_BuildValue("{s:L,s:L,s:s,s:s,s:I,s:s,s:I,s:s,s:s}",
                                      "start_index", start_idx,
                                      "start_time", rh->start_time,
                                      "start_time_string", check_utf8(time_str),
                                      "type_string", check_utf8(rh->type_string),
                                      "type_code", rh->type_code,
                                      "version_string", check_utf8(ver_str),
                                      "encryption", enc_level,
                                      "encryption_string", check_utf8(enc_str),
                                      "text", enc_level <= 0 ? check_utf8((si1 *) rh + RECORD_HEADER_BYTES_m12) : "<no access>");
            break;
        case REC_NlxP_TYPE_CODE_m12:
            NlxP = (REC_NlxP_v10_m12 *) ((ui1 *) rh + RECORD_HEADER_BYTES_m12);
            if (enc_level <= 0) {
                py_record = Py_BuildValue("{s:L,s:L,s:s,s:s,s:I,s:s,s:I,s:s,s:I,s:I}",
                                          "start_index", start_idx,
                                          "start_time", rh->start_time,
                                          "start_time_string", check_utf8(time_str),
                                          "type_string", check_utf8(rh->type_string),
                                          "type_code", rh->type_code,
                                          "version_string", check_utf8(ver_str),
                                          "encryption", enc_level,
                                          "encryption_string", check_utf8(enc_str),
                                          "subport", NlxP->subport,
                                          "value", NlxP->value);
            } else {
                py_record = Py_BuildValue("{s:L,s:L,s:s,s:s,s:I,s:s,s:I,s:s,s:s,s:s}",
                                          "start_index", start_idx,
                                          "start_time", rh->start_time,
                                          "start_time_string", check_utf8(time_str),
                                          "type_string", check_utf8(rh->type_string),
                                          "type_code", rh->type_code,
                                          "version_string", check_utf8(ver_str),
                                          "encryption", enc_level,
                                          "encryption_string", check_utf8(enc_str),
                                          "subport", "<no access>",
                                          "value", "<no access>");
            }
            break;
        case REC_ESti_TYPE_CODE_m12:
            ESti = (REC_ESti_v10_m12 *) ((ui1 *) rh + RECORD_HEADER_BYTES_m12);
            if (enc_level <= 0) {
                py_record = Py_BuildValue("{s:L,s:L,s:s,s:s,s:I,s:s,s:I,s:s,s:d,s:d,s:L,s:I,s:I,s:s,s:s,s:s}",
                                          "start_idx", start_idx,
                                          "start_time", rh->start_time,
                                          "start_time_string", check_utf8(time_str),
                                          "type_string", check_utf8(rh->type_string),
                                          "type_code", rh->type_code,
                                          "version_string", check_utf8(ver_str),
                                          "encryption", enc_level,
                                          "encryption_string", check_utf8(enc_str),
                                          "amplitude", ESti->amplitude,
                                          "frequency", ESti->frequency,
                                          "pulse_width", ESti->pulse_width,
                                          "amp_unit_code", ESti->amp_unit_code,
                                          "mode_code", ESti->mode_code,
                                          "waveform", check_utf8(ESti->waveform),
                                          "anode", check_utf8(ESti->anode),
                                          "cathode", check_utf8(ESti->cathode));
            } else {
                py_record = Py_BuildValue("{s:L,s:L,s:s,s:s,s:I,s:s,s:I,s:s,s:s,s:s,s:s,s:s,s:s,s:s,s:s,s:s}",
                                          "start_idx", start_idx,
                                          "start_time", rh->start_time,
                                          "start_time_string", check_utf8(time_str),
                                          "type_string", check_utf8(rh->type_string),
                                          "type_code", rh->type_code,
                                          "version_string", check_utf8(ver_str),
                                          "encryption", enc_level,
                                          "encryption_string", check_utf8(enc_str),
                                          "amplitude", "<no access>",
                                          "frequency", "<no access>",
                                          "pulse_width", "<no access>",
                                          "amp_unit_code", "<no access>",
                                          "mode_code", "<no access>",
                                          "waveform", "<no access>",
                                          "anode", "<no access>",
                                          "cathode", "<no access>");
            }
            break;
        case REC_CSti_TYPE_CODE_m12:
            CSti = (REC_CSti_v10_m12 *) ((ui1 *) rh + RECORD_HEADER_BYTES_m12);
            if (enc_level <= 0) {
                py_record = Py_BuildValue("{s:L,s:L,s:s,s:s,s:I,s:s,s:I,s:s,s:L,s:s,s:s,s:s}",
                                          "start_index", start_idx,
                                          "start_time", rh->start_time,
                                          "start_time_string", check_utf8(time_str),
                                          "type_string", check_utf8(rh->type_string),
                                          "type_code", rh->type_code,
                                          "version_string", check_utf8(ver_str),
                                          "encryption", enc_level,
                                          "encryption_string", check_utf8(enc_str),
                                          "stimulus_duration", CSti->stimulus_duration,
                                          "task_type", check_utf8(CSti->task_type),
                                          "stimulus_type", check_utf8(CSti->stimulus_type),
                                          "patient_response", check_utf8(CSti->patient_response));
            } else {
                py_record = Py_BuildValue("{s:L,s:L,s:s,s:s,s:I,s:s,s:I,s:s,s:s,s:s,s:s,s:s}",
                                          "start_index", start_idx,
                                          "start_time", rh->start_time,
                                          "start_time_string", check_utf8(time_str),
                                          "type_string", check_utf8(rh->type_string),
                                          "type_code", rh->type_code,
                                          "version_string", check_utf8(ver_str),
                                          "encryption", enc_level,
                                          "encryption_string", check_utf8(enc_str),
                                          "stimulus_duration", "<no access>",
                                          "task_type", "<no access>",
                                          "stimulus_type", "<no access>",
                                          "patient_response", "<no access>");
            }
            break;
        case REC_Sgmt_TYPE_CODE_m12:
            Sgmt = (REC_Sgmt_v10_m12 *) ((ui1 *) rh + RECORD_HEADER_BYTES_m12);
            if (enc_level <= 0) {
                STR_time_string_m12(Sgmt->end_time, time_str_end, TRUE_m12, relative_days, FALSE_m12);
                
                valid_text = 0;
                if (rh->total_record_bytes > (RECORD_HEADER_BYTES_m12 + REC_Sgmt_v10_BYTES_m12)) {
                    text = (si1 *) rh + RECORD_HEADER_BYTES_m12 + REC_Sgmt_v10_BYTES_m12;
                    if (*text)
                        valid_text = 1;
                }

                if (Sgmt->acquisition_channel_number != REC_Sgmt_v10_ACQUISITION_CHANNEL_NUMBER_ALL_CHANNELS_m12) {
                    if (Sgmt->sampling_frequency != REC_Sgmt_v10_SAMPLING_FREQUENCY_VARIABLE_m12) {
                        py_record = Py_BuildValue("{s:L,s:L,s:s,s:s,s:I,s:s,s:I,s:s,s:L,s:s,s:L,s:L,s:I,s:K,s:I,s:d,s:s}",
                                                  "start_index", start_idx,
                                                  "start_time", rh->start_time,
                                                  "start_time_string", check_utf8(time_str),
                                                  "type_string", check_utf8(rh->type_string),
                                                  "type_code", rh->type_code,
                                                  "version_string", check_utf8(ver_str),
                                                  "encryption", enc_level,
                                                  "encryption_string", check_utf8(enc_str),
                                                  "end_time", Sgmt->end_time,
                                                  "end_time_string", check_utf8(time_str_end),
                                                  "start_sample_number", Sgmt->start_sample_number,
                                                  "end_sample_number", Sgmt->end_sample_number,
                                                  "segment_number", Sgmt->segment_number,
                                                  "segment_UID", Sgmt->segment_UID,
                                                  "acquistion_channel_number", Sgmt->acquisition_channel_number,
                                                  "sampling_frequency", Sgmt->sampling_frequency,
                                                  "description", valid_text == 1 ? check_utf8(text) : "<no description>");
                    } else {
                        py_record = Py_BuildValue("{s:L,s:L,s:s,s:s,s:I,s:s,s:I,s:s,s:L,s:s,s:L,s:L,s:I,s:K,s:I,s:s,s:s}",
                                                  "start_index", start_idx,
                                                  "start_time", rh->start_time,
                                                  "start_time_string", check_utf8(time_str),
                                                  "type_string", check_utf8(rh->type_string),
                                                  "type_code", rh->type_code,
                                                  "version_string", check_utf8(ver_str),
                                                  "encryption", enc_level,
                                                  "encryption_string", check_utf8(enc_str),
                                                  "end_time", Sgmt->end_time,
                                                  "end_time_string", check_utf8(time_str_end),
                                                  "start_sample_number", Sgmt->start_sample_number,
                                                  "end_sample_number", Sgmt->end_sample_number,
                                                  "segment_number", Sgmt->segment_number,
                                                  "segment_UID", Sgmt->segment_UID,
                                                  "acquistion_channel_number", Sgmt->acquisition_channel_number ,
                                                  "sampling_frequency", "variable",
                                                  "description", valid_text == 1 ? check_utf8(text) : "<no description>");
                    }
                } else {
                    if (Sgmt->sampling_frequency != REC_Sgmt_v10_SAMPLING_FREQUENCY_VARIABLE_m12) {
                        py_record = Py_BuildValue("{s:L,s:L,s:s,s:s,s:I,s:s,s:I,s:s,s:L,s:s,s:L,s:L,s:I,s:K,s:s,s:d,s:s}",
                                                  "start_index", start_idx,
                                                  "start_time", rh->start_time,
                                                  "start_time_string", check_utf8(time_str),
                                                  "type_string", check_utf8(rh->type_string),
                                                  "type_code", rh->type_code,
                                                  "version_string", check_utf8(ver_str),
                                                  "encryption", enc_level,
                                                  "encryption_string", check_utf8(enc_str),
                                                  "end_time", Sgmt->end_time,
                                                  "end_time_string", check_utf8(time_str_end),
                                                  "start_sample_number", Sgmt->start_sample_number,
                                                  "end_sample_number", Sgmt->end_sample_number,
                                                  "segment_number", Sgmt->segment_number,
                                                  "segment_UID", Sgmt->segment_UID,
                                                  "acquistion_channel_number", "all channels",
                                                  "sampling_frequency", Sgmt->sampling_frequency,
                                                  "description", valid_text == 1 ? check_utf8(text) : "<no description>");
                    } else {
                        py_record = Py_BuildValue("{s:L,s:L,s:s,s:s,s:I,s:s,s:I,s:s,s:L,s:s,s:L,s:L,s:I,s:K,s:s,s:s,s:s}",
                                                  "start_index", start_idx,
                                                  "start_time", rh->start_time,
                                                  "start_time_string", check_utf8(time_str),
                                                  "type_string", check_utf8(rh->type_string),
                                                  "type_code", rh->type_code,
                                                  "version_string", check_utf8(ver_str),
                                                  "encryption", enc_level,
                                                  "encryption_string", check_utf8(enc_str),
                                                  "end_time", Sgmt->end_time,
                                                  "end_time_string", check_utf8(time_str_end),
                                                  "start_sample_number", Sgmt->start_sample_number,
                                                  "end_sample_number", Sgmt->end_sample_number,
                                                  "segment_number", Sgmt->segment_number,
                                                  "segment_UID", Sgmt->segment_UID,
                                                  "acquistion_channel_number", "all channels",
                                                  "sampling_frequency", "variable",
                                                  "description", valid_text == 1 ? check_utf8(text) : "<no description>");
                    }
                }
            }
            else {
                py_record = Py_BuildValue("{s:L,s:L,s:s,s:s,s:I,s:s,s:I,s:s,s:s,s:s,s:s,s:s,s:s,s:s,s:s,s:s,s:s}",
                                          "start_index", start_idx,
                                          "start_time", rh->start_time,
                                          "start_time_string", check_utf8(time_str),
                                          "type_string", check_utf8(rh->type_string),
                                          "type_code", rh->type_code,
                                          "version_string", check_utf8(ver_str),
                                          "encryption", enc_level,
                                          "encryption_string", check_utf8(enc_str),
                                          "end_time", "<no access>",
                                          "end_time_string", "<no access>",
                                          "start_sample_number", "<no access>",
                                          "end_sample_number", "<no access>",
                                          "segment_number", "<no access>",
                                          "segment_UID", "<no access>",
                                          "acquistion_channel_number", "<no access>",
                                          "sampling_frequency", "<no access>",
                                          "description", "<no access>");
                
            }
            
            break;
        default:
            py_record = Py_BuildValue("{s:L,s:L,s:s,s:s,s:I,s:s,s:s,s:s}",
                                      "start_index", start_idx,
                                      "start_time", rh->start_time,
                                      "start_time_string", check_utf8(time_str),
                                      "type_string", check_utf8(rh->type_string),
                                      "type_code", rh->type_code,
                                      "version_string", check_utf8(ver_str),
                                      "encryption", enc_level,
                                      "encryption_string", check_utf8(enc_str),
                                      "comment", enc_level <= 0 ? " unknown record type" : "<no access>");
            break;
    }
    return py_record;
    
}


si4     rec_compare(const void *a, const void *b)
{
    si8    time_d;
    
    time_d = (*((RECORD_HEADER_m12 **) a))->start_time - (*((RECORD_HEADER_m12 **) b))->start_time;
    
    // sort by time
    if (time_d > 0)
        return(1);
    if (time_d < 0)
        return(-1);
    
    // if same time, sort by location in memory
    if ((ui8) *((RECORD_HEADER_m12 **) a) > (ui8) *((RECORD_HEADER_m12 **) a))
        return(1);
    
    return(-1);
}


PyObject            *get_dm(PyObject *self, PyObject *args)
{

    SESSION_m12             *sess;
    PyObject                *sess_capsule_object, *dm_capsule_obj;
    PyObject                *start_time_input_obj, *end_time_input_obj;
    PyObject                *start_index_input_obj, *end_index_input_obj;
    PyObject                *n_out_samps_obj, *major_dimension_obj;
    PyObject                *sampling_frequency_obj;
    PyObject                *return_records_obj;
    si8                     n_out_samps;
    PyObject                *temp_UTF_str;
    si1                     *temp_str_bytes;
    sf8 sampling_frequency;
    PyArrayObject                           *py_array_out;

    si1            time_str_start[TIME_STRING_BYTES_m12];
    si1            time_str_end[TIME_STRING_BYTES_m12];
    si8            i, k;
    TIME_SLICE_m12        *slice, local_time_slice;
    si4            seg_idx;

    DATA_MATRIX_m12         *dm;
    PyObject *py_sampling_frequencies;
    PyObject *py_channel_names;
    PyObject *py_records;
    //PyObject *py_minima;
    //PyObject *py_maxima;
    PyObject *raw_page;
    PyObject *py_contigua;
    PyObject *py_float_object;
    PyObject *py_string_object;
    PyArrayObject *mins, *maxs;
    npy_intp dims[2];
    TERN_m12   channel_major, return_records;
    si4 n_chans, n_active_chans;


    // initialize Numpy
    import_array();

    // set defaults for arguements
    start_index_input_obj = NULL;
    end_index_input_obj = NULL;
    major_dimension_obj = NULL;
    start_time_input_obj = NULL;
    end_time_input_obj = NULL;
    n_out_samps_obj = NULL;
    sampling_frequency_obj = NULL;
    return_records_obj = NULL;



    // --- Parse the input ---
    if (!PyArg_ParseTuple(args,"OO|OOOOOOO",
                          &sess_capsule_object,
                          &dm_capsule_obj,
                          &start_index_input_obj,
                          &end_index_input_obj,
                          &start_time_input_obj,
                          &end_time_input_obj,
                          &n_out_samps_obj,
                          &sampling_frequency_obj,
                          &return_records_obj)){

        PyErr_SetString(PyExc_RuntimeError, "15 inputs: pointers, start_time, end_time, n_out_samps\n");
        PyErr_Occurred();
        return NULL;
    }

    // Get session struct from python object
    sess = (SESSION_m12*) PyCapsule_GetPointer(sess_capsule_object, "session");
    dm = (DATA_MATRIX_m12*) PyCapsule_GetPointer(dm_capsule_obj, "dm");

    // set defaults, in case args are NULL/None
    sampling_frequency = FREQUENCY_NO_ENTRY_m12;
    n_out_samps = NUMBER_OF_SAMPLES_NO_ENTRY_m12;
    return_records = TRUE_m12;

    slice = &local_time_slice; // this takes care of freeing memory
    G_initialize_time_slice_m12(slice);
    initialize_time_slice(slice, start_index_input_obj, end_index_input_obj, start_time_input_obj, end_time_input_obj);
    //G_show_time_slice_m12(slice);

    // process n_out_samps
    if (n_out_samps_obj != NULL)
    {
        if (PyNumber_Check(n_out_samps_obj)) {
            PyObject* number = PyNumber_Long(n_out_samps_obj);
            n_out_samps = PyLong_AsLongLong(number);
            // make end_time not inclusive, by adjust by 1 microsecond.

        } else if (n_out_samps_obj == Py_None) {
            n_out_samps = NUMBER_OF_SAMPLES_NO_ENTRY_m12;
        } else {
            PyErr_SetString(PyExc_RuntimeError, "N_out_samps (input 4) must be specified as an integer\n");
            PyErr_Occurred();
            return NULL;
        }
    }

    // process sampling_frequency
    if (sampling_frequency_obj != NULL)
    {
        if (PyNumber_Check(sampling_frequency_obj)) {
            PyObject* number = PyNumber_Long(sampling_frequency_obj);
            sampling_frequency = (double)(PyLong_AsLong(number));
            // make end_time not inclusive, by adjust by 1 microsecond.
        } else if (PyFloat_Check(sampling_frequency_obj)) {
            PyObject* number = PyNumber_Float(sampling_frequency_obj);
            sampling_frequency = PyFloat_AsDouble(number);
            // make end_time not inclusive, by adjust by 1 microsecond.
        } else if (sampling_frequency_obj == Py_None) {
            // if no output length is specified, then default to max channel's freq
            if (n_out_samps == NUMBER_OF_SAMPLES_NO_ENTRY_m12)
                sampling_frequency = DM_MAXIMUM_INPUT_FREQUENCY_m12;
            else
                sampling_frequency = FREQUENCY_NO_ENTRY_m12;
        } else {
            PyErr_SetString(PyExc_RuntimeError, "sampling_frequency (input 7) must be specified as a float\n");
            PyErr_Occurred();
            return NULL;
        }
    }

//    printf("sampling_frequency=%f\n", sampling_frequency);

    if (return_records_obj != NULL)
    {
        if (PyBool_Check(return_records_obj)) {
            if (PyObject_IsTrue(return_records_obj)){
                return_records = TRUE_m12;
                }
            else{
                return_records = FALSE_m12;
                }
        } else {
            PyErr_SetString(PyExc_RuntimeError, "return_records (input 8) must be specified as a boolean\n");
            PyErr_Occurred();
            return NULL;
        }
    }

    n_chans = sess->number_of_time_series_channels;

    // Get data matrix pointer

    // Create DM matrix structure
    dm->channel_count = n_chans;
    dm->sample_count = n_out_samps;
    dm->sampling_frequency = sampling_frequency;
    //dm->data_bytes = (n_out_samps * n_chans) << 3;
    dm->data_bytes = 0;
    dm->data = NULL;
    dm->range_minima = NULL;
    dm->range_maxima = NULL;
//    printf("samps=%d freq=%f\n",n_out_samps, sampling_frequency);

    // Build matrix
    dm = DM_get_matrix_m12(dm, sess, slice, FALSE_m12);

    if (dm == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "get_matrix returned NULL!\n");
        PyErr_Occurred();
        return(NULL);
    }
    //show_time_slice_m12(&sess->time_slice);

    if (dm->flags & DM_FMT_CHANNEL_MAJOR_m12) {
        dims[0] = dm->channel_count;
        dims[1] = dm->sample_count;
    }
    else {
        dims[0] = dm->sample_count;
        dims[1] = dm->channel_count;
    }
    //printf ("%d %d\n", n_chans, n_out_samps);

    // Build contigua if desired
    if (dm->flags & DM_DSCNT_CONTIG_m12)
        py_contigua = build_contigua_dm(dm);
    else
        py_contigua = Py_None;

    STR_time_string_m12(sess->time_slice.start_time, time_str_start, TRUE_m12, FALSE_m12, FALSE_m12);
    STR_time_string_m12(sess->time_slice.end_time, time_str_end, TRUE_m12, FALSE_m12, FALSE_m12);

    n_active_chans = dm->channel_count;

    // Fill in channel names and sampling frequencies
    py_sampling_frequencies = PyList_New(n_active_chans);
    py_channel_names = PyList_New(n_active_chans);
    slice = &sess->time_slice;
    seg_idx = G_get_segment_index_m12(slice->start_segment_number);
    for (i = k = 0; k < n_chans; ++k) {
        if ((sess->time_series_channels[k]->flags & LH_CHANNEL_ACTIVE_m12) == 0)
            continue;

        py_float_object = PyFloat_FromDouble(sess->time_series_channels[k]->segments[seg_idx]->metadata_fps->metadata->time_series_section_2.sampling_frequency);
        PyList_SetItem(py_sampling_frequencies, i, py_float_object);

        py_string_object = PyUnicode_FromString(sess->time_series_channels[k]->name);
        PyList_SetItem(py_channel_names, i, py_string_object);

        i++;
    }

    // session records
    if (return_records == TRUE_m12)
        py_records = fill_session_records(sess, dm);
    else
        py_records = Py_None;


    py_array_out = (PyArrayObject *) PyArray_SimpleNewFromData(2, dims, NPY_DOUBLE, (void *) dm->data);
    // The following line is necessary to tell NumPy that it now owns the data.  Without that line,
    // the array will never get garbage-collected.
    PyArray_ENABLEFLAGS((PyArrayObject*) py_array_out, NPY_ARRAY_OWNDATA);

    if (dm->flags & DM_TRACE_RANGES_m12) {
        mins = (PyArrayObject *) PyArray_SimpleNewFromData(2, dims, NPY_DOUBLE, (void *) dm->range_minima);
        // The following line is necessary to tell NumPy that it now owns the data.  Without that line,
        // the array will never get garbage-collected.
        PyArray_ENABLEFLAGS((PyArrayObject*) mins, NPY_ARRAY_OWNDATA);
        maxs = (PyArrayObject *) PyArray_SimpleNewFromData(2, dims, NPY_DOUBLE, (void *) dm->range_maxima);
        // The following line is necessary to tell NumPy that it now owns the data.  Without that line,
        // the array will never get garbage-collected.
        PyArray_ENABLEFLAGS((PyArrayObject*) maxs, NPY_ARRAY_OWNDATA);
    }

    raw_page = Py_BuildValue("{s:L,s:s,s:L,s:s,s:O,s:O,s:O,s:O,s:O,s:O,s:O,s:f,s:L,s:L}",
                             "start_time", slice->start_time,
                             "start_time_string", check_utf8(time_str_start),
                             "end_time", slice->end_time,
                             "end_time_string", check_utf8(time_str_end),
                             "channel_names", py_channel_names,
                             "channel_sampling_frequencies", py_sampling_frequencies,
                             "contigua", py_contigua,
                             "records", py_records,
                             "samples", py_array_out,
                             "minima", (dm->flags & DM_TRACE_RANGES_m12) ? mins : Py_None,
                             "maxima", (dm->flags & DM_TRACE_RANGES_m12) ? maxs : Py_None,
                             "sampling_frequency", dm->sampling_frequency,
                             "sample_count", dm->sample_count,
                             "channel_count", dm->channel_count);


    // Clean up
    Py_DECREF(py_channel_names);
    Py_DECREF(py_sampling_frequencies);
    Py_DECREF(py_records);
    Py_DECREF(py_contigua);
    Py_DECREF(py_array_out);
    if (dm->flags & DM_TRACE_RANGES_m12) {
        Py_DECREF(mins);
        Py_DECREF(maxs);
    }

    // free DM matrix structure
    dm->data = dm->range_minima = dm->range_maxima = NULL;  // keep python allocated memory

    return raw_page;
}

PyObject*    build_contigua_dm(DATA_MATRIX_m12 *dm)
{
    extern GLOBALS_m12        *globals_m12;
    TERN_m12            relative_days;
    si1                             time_str_start[TIME_STRING_BYTES_m12];
    si1                             time_str_end[TIME_STRING_BYTES_m12];
    si8                             i, n_contigs;
    CONTIGUON_m12            *contigua;
    PyObject  *py_contiguon;
    PyObject  *py_contigua_list;
    
    
    n_contigs = dm->number_of_contigua;
    
    py_contigua_list = PyList_New(n_contigs);
    
    if (n_contigs <= 0)
        return py_contigua_list;
    
    if (globals_m12->RTO_known == TRUE_m12)
        relative_days = FALSE_m12;
    else
        relative_days = TRUE_m12;
    
    contigua = dm->contigua;
    
    for (i = 0; i < n_contigs; ++i) {
        
        // start time string
        STR_time_string_m12(contigua[i].start_time, time_str_start, TRUE_m12, relative_days, FALSE_m12);
        // end time string
        STR_time_string_m12(contigua[i].end_time, time_str_end, TRUE_m12, relative_days, FALSE_m12);
        
        py_contiguon = Py_BuildValue("{s:L,s:L,s:L,s:s,s:L,s:s}",
                                     "start_index", contigua[i].start_sample_number,
                                     "end_index", contigua[i].end_sample_number,
                                     "start_time", contigua[i].start_time,
                                     "start_time_string", time_str_start,
                                     "end_time", contigua[i].end_time,
                                     "end_Time_string", check_utf8(time_str_end));
        
        PyList_SetItem(py_contigua_list, i, py_contiguon);
    }

    return py_contigua_list;
}


PyObject *sort_channels_by_acq_num(PyObject *self, PyObject *args)
{
    SESSION_m12             *sess;
    PyObject                *sess_capsule_object;
    
    // --- Parse the input ---
    if (!PyArg_ParseTuple(args,"O",
                          &sess_capsule_object)){
        
        PyErr_SetString(PyExc_RuntimeError, "1 inputs required: pointers\n");
        PyErr_Occurred();
        return NULL;
    }
    
    sess = PyCapsule_GetPointer(sess_capsule_object, "session");

    if (sess == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Session pointer is NULL\n");
        PyErr_Occurred();
        return NULL;
    }
    
    G_sort_channels_by_acq_num_m12(sess);
    
    Py_INCREF(Py_None);
    return Py_None;
    
}

PyObject *read_lh_flags(PyObject *self, PyObject *args)
{
    SESSION_m12             *sess;
    CHANNEL_m12             *chan;
    SEGMENT_m12             *seg;
    TIME_SLICE_m12          *slice;
    PyObject                *sess_capsule_object;
    PyObject                *py_session_dict;
    PyObject                *py_channels_dict, *py_channel_dict;
    PyObject                *py_segments_dict, *py_segment_dict;
    PyObject                *py_flags_list, *bit_list, *bit_value;
    si8                     i, j, m;
    si4                     bit, n_bits, n_channels, n_segs;
    ui8                     flags;

    // --- Parse the input ---
    if (!PyArg_ParseTuple(args,"O",
                          &sess_capsule_object)){

        PyErr_SetString(PyExc_RuntimeError, "input required: pointers\n");
        PyErr_Occurred();
        return NULL;
    }

    py_session_dict = Py_None;

    // Get session struct from python object
    sess = PyCapsule_GetPointer(sess_capsule_object, "session");

    // Create session dict and set session level flags
    flags = sess->header.flags;
    n_bits = sizeof(ui8) * 8;
    bit_list = PyList_New(n_bits);
    for (m=0; m<n_bits; ++m) {
        bit = (flags >> m) & 1;  // Extract the i-th bit
        bit_value = PyLong_FromLong(bit);
        PyList_SetItem(bit_list, m, bit_value);
    }
    py_session_dict = Py_BuildValue("{s:O}",
                                    "session_flags", bit_list);

    n_channels = sess->number_of_time_series_channels;

    py_channels_dict = PyDict_New();
    for (i=0; i<sess->number_of_time_series_channels; ++i) {
        chan = sess->time_series_channels[i];
        flags = chan->header.flags;
        slice = &chan->time_slice;

        if (slice->number_of_segments == UNKNOWN_m12) {
            n_segs = G_get_segment_range_m12((LEVEL_HEADER_m12 *) chan, slice);
        } else {
            n_segs = slice->number_of_segments;
        }

        // Create channel dict and set channel level flags
        n_bits = sizeof(ui8) * 8;
        bit_list = PyList_New(n_bits);
        for (m=0; m<n_bits; ++m) {
            bit = (flags >> m) & 1;  // Extract the i-th bit
            bit_value = PyLong_FromLong(bit);
            PyList_SetItem(bit_list, m, bit_value);
        }
        py_channel_dict = Py_BuildValue("{s:O}",
                                        "channel_flags", bit_list);

        py_segments_dict = PyDict_New();
        for (j=0; j<n_segs; ++j) {
            seg = chan->segments[j];
            flags = seg->header.flags;
            // Create segment dict and set segment level flags
            n_bits = sizeof(ui8) * 8;
            bit_list = PyList_New(n_bits);
            for (m=0; m<n_bits; ++m) {
                bit = (flags >> m) & 1;  // Extract the i-th bit
                bit_value = PyLong_FromLong(bit);
                PyList_SetItem(bit_list, m, bit_value);
            }
            py_segment_dict = Py_BuildValue("{s:O}",
                                            "segment_flags", bit_list);


            // Set one segment to segments dict
            PyDict_SetItemString(py_segments_dict, seg->name, py_segment_dict);
        }

        // Set segments to channel dict
        PyDict_SetItemString(py_channel_dict, "segments", py_segments_dict);

        // Set one channel to channels dict
        PyDict_SetItemString(py_channels_dict, chan->name, py_channel_dict);
    }

    // Set channels to session dict
    PyDict_SetItemString(py_session_dict, "channels", py_channels_dict);

    return py_session_dict;
}


PyObject *push_lh_flags(PyObject *self, PyObject *args)
{
    SESSION_m12             *sess;
    CHANNEL_m12             *chan;
    SEGMENT_m12             *seg;
    TIME_SLICE_m12          *slice;
    PyObject                *sess_capsule_object;
    PyObject                *py_channels_dict, *py_channel_dict;
    PyObject                *py_segments_dict, *py_segment_dict;
    PyObject                *flags_dict, *bit_list, *bit_value;
    ui8                     flags;
    si8                     i, j, m;
    si4                     bit, n_bits, n_segs;

    // --- Parse the input ---
    if (!PyArg_ParseTuple(args,"OO",
                          &sess_capsule_object,
                          &flags_dict)){

        PyErr_SetString(PyExc_RuntimeError, "input required: pointers and flags dict\n");
        PyErr_Occurred();
        return NULL;
    }

    // Get session struct from python object
    sess = PyCapsule_GetPointer(sess_capsule_object, "session");

    // Set session level flags
    bit_list = PyDict_GetItemString(flags_dict, "session_flags");
    if (bit_list == NULL){
        PyErr_SetString(PyExc_RuntimeError, "Key session_flags not found in flags dictionary\n");
        PyErr_Occurred();
        return NULL;
    }

    n_bits = sizeof(ui8) * 8;

    flags = 0;
    for (m = 0; m < n_bits; m++) {
        bit_value = PyList_GetItem(bit_list, m);  // Get the i-th item
        if (!PyLong_Check(bit_value)) {
            PyErr_SetString(PyExc_TypeError, "List items must be integers.");
            return NULL;
        }
        bit = PyLong_AsLong(bit_value);  // Convert Python integer to C long

        // Validate bit value (must be either 0 or 1)
        if (bit < 0 || bit > 1) {
            PyErr_SetString(PyExc_ValueError, "Bits must be 0 or 1.");
            return NULL;
        }
        if (bit == 1) {
            flags |= (1ULL << m); // Set the bit at the correct position
        }
    }

    sess->header.flags = flags;

    // Check if channels are in session dictionary
    py_channels_dict = PyDict_GetItemString(flags_dict, "channels");
    if (py_channels_dict == NULL){
        return Py_None;
    }

    for (i=0; i<sess->number_of_time_series_channels; ++i) {
        chan = sess->time_series_channels[i];
        slice = &chan->time_slice;

        // Check if the channel is in the channels dictionary
        py_channel_dict = PyDict_GetItemString(py_channels_dict, chan->name);
        if (py_channel_dict == NULL){
            continue;
        }

        // Set channel flags if they are present
        bit_list = PyDict_GetItemString(py_channel_dict, "channel_flags");
        if (bit_list == NULL){
            continue;
        }else{
            flags = 0;
            for (m = 0; m < n_bits; m++) {
                PyObject* bit_value = PyList_GetItem(bit_list, m);  // Get the i-th item
                if (!PyLong_Check(bit_value)) {
                    PyErr_SetString(PyExc_TypeError, "List items must be integers.");
                    return NULL;
                }

                bit = PyLong_AsLong(bit_value);  // Convert Python integer to C long

                // Validate bit value (must be either 0 or 1)
                if (bit < 0 || bit > 1) {
                    PyErr_SetString(PyExc_ValueError, "Bits must be 0 or 1.");
                    return NULL;
                }
                if (bit == 1) {
                    flags |= (1ULL << m); // Set the bit at the correct position
                }
            }
            chan->header.flags = flags;
        }

        // Check if segments are in the channel dictionary
        py_segments_dict = PyDict_GetItemString(py_channel_dict, "segments");
        if (py_segments_dict == NULL){
            continue;
        }

        if (slice->number_of_segments == UNKNOWN_m12) {
            n_segs = G_get_segment_range_m12((LEVEL_HEADER_m12 *) chan, slice);
        } else {
            n_segs = slice->number_of_segments;
        }

        for (j=0; j<n_segs; ++j) {
            seg = chan->segments[j];

            // Check if the segment is in the segments dictionary
            py_segment_dict = PyDict_GetItemString(py_segments_dict, seg->name);
            if (py_segment_dict == NULL){
                continue;
            }

            // Set segment flags if they are present
            bit_list = PyDict_GetItemString(py_channel_dict, "segment_flags");
            if (bit_list == NULL){
                continue;
            }else{
                flags=0;
                for (m = 0; m < n_bits; m++) {
                    PyObject* bit_value = PyList_GetItem(bit_list, m);  // Get the i-th item
                    if (!PyLong_Check(bit_value)) {
                        PyErr_SetString(PyExc_TypeError, "List items must be integers.");
                        return NULL;
                    }

                    bit = PyLong_AsLong(bit_value);  // Convert Python integer to C long

                    // Validate bit value (must be either 0 or 1)
                    if (bit < 0 || bit > 1) {
                        PyErr_SetString(PyExc_ValueError, "Bits must be 0 or 1.");
                        return NULL;
                    }
                    if (bit == 1) {
                        flags |= (1ULL << m); // Set the bit at the correct position
                    }
                }
                seg->header.flags = flags;
            }
        }
    }

    return Py_None;
}

PyObject *read_dm_flags(PyObject *self, PyObject *args)
{
    DATA_MATRIX_m12         *dm;
    PyObject                *dm_capsule_obj;
    PyObject                *py_dm_dict;
    PyObject                *bit_value, *bit_list;
    si4                     m, bit, n_bits;
    ui8                     flags;


    // --- Parse the input ---
    if (!PyArg_ParseTuple(args,"O",
                          &dm_capsule_obj)){

        PyErr_SetString(PyExc_RuntimeError, "input required: pointers\n");
        PyErr_Occurred();
        return NULL;
    }

    py_dm_dict = Py_None;

    // Get data_matrix struct from python object
    dm = (DATA_MATRIX_m12 *) PyCapsule_GetPointer(dm_capsule_obj, "dm");

    // Create session dict and set session level flags
    flags = dm->flags;
    n_bits = sizeof(ui8) * 8;
    bit_list = PyList_New(n_bits);
    for (m=0; m<n_bits; ++m) {
        bit = (flags >> m) & 1;  // Extract the i-th bit
        bit_value = PyLong_FromLong(bit);
        PyList_SetItem(bit_list, m, bit_value);
    }

    py_dm_dict = Py_BuildValue("{s:O}",
                               "data_matrix_flags", bit_list);

    return py_dm_dict;
}

PyObject *push_dm_flags(PyObject *self, PyObject *args)
{
    DATA_MATRIX_m12         *dm;
    PyObject                *dm_capsule_obj;
    PyObject                *bit_list, *bit_value;
    PyObject                *flags_dict;
    ui8                     flags;
    si4                     m, bit, n_bits;

    // --- Parse the input ---
    if (!PyArg_ParseTuple(args,"OO",
                          &dm_capsule_obj,
                          &flags_dict)){

        PyErr_SetString(PyExc_RuntimeError, "input required: pointers and flags dict\n");
        PyErr_Occurred();
        return NULL;
    }

    // Get data_matrix struct from python object
    dm = (DATA_MATRIX_m12 *) PyCapsule_GetPointer(dm_capsule_obj, "dm");

    // Set session level flags
    bit_list = PyDict_GetItemString(flags_dict, "data_matrix_flags");
    if (bit_list == NULL){
        PyErr_SetString(PyExc_RuntimeError, "Key data_matrix_flags not found in flags dictionary\n");
        PyErr_Occurred();
        return NULL;
    }

    n_bits = sizeof(ui8) * 8;

    flags = 0;
    for (m = 0; m < n_bits; m++) {
        bit_value = PyList_GetItem(bit_list, m);  // Get the i-th item
        if (!PyLong_Check(bit_value)) {
            PyErr_SetString(PyExc_TypeError, "List items must be integers.");
            return NULL;
        }
        bit = PyLong_AsLong(bit_value);  // Convert Python integer to C long

        // Validate bit value (must be either 0 or 1)
        if (bit < 0 || bit > 1) {
            PyErr_SetString(PyExc_ValueError, "Bits must be 0 or 1.");
            return NULL;
        }
        if (bit == 1) {
            flags |= (1ULL << m); // Set the bit at the correct position
        }
    }

    dm->flags = flags;

    return Py_None;
}

//PyObject *set_single_channel_active(PyObject *self, PyObject *args)
//{
//    SESSION_m12             *sess;
//    TERN_m12                all, none;
//    TERN_m12                is_active;
//    PyObject                *pointers_obj, *chan_name_obj, *is_active_obj;
//    PyObject                *temp_UTF_str;
//    si1                     *temp_str_bytes;
//    PyObject                *seq;
//    PyObject                *item;
//    si1                     chan_name[BASE_FILE_NAME_BYTES_m12];
//    si8                     i, n_active_chans;
//    CHANNEL_m12             *chan;
//    si1                     reference_chan_temp[BASE_FILE_NAME_BYTES_m12];
//
//
//
//    pointers_obj = NULL;
//    chan_name_obj = NULL;
//    is_active_obj = NULL;
//
//    // --- Parse the input ---
//    if (!PyArg_ParseTuple(args,"OOO",
//                          &pointers_obj,
//                          &chan_name_obj,
//                          &is_active_obj)){
//
//        PyErr_SetString(PyExc_RuntimeError, "3 inputs required: pointers, chan_name, is_active\n");
//        PyErr_Occurred();
//        return NULL;
//    }
//
//    all = FALSE_m12;
//    none = FALSE_m12;
//
//    if (chan_name_obj != NULL)
//    {
//        if (PyUnicode_Check(chan_name_obj)) {
//            temp_UTF_str = PyUnicode_AsEncodedString(chan_name_obj, "utf-8","strict"); // Encode to UTF-8 python objects
//            temp_str_bytes = PyBytes_AS_STRING(temp_UTF_str); // Get the *char
//
//            if (!*temp_str_bytes) {
//                PyErr_SetString(PyExc_RuntimeError, "chan_name (input 2) can be specified as a string\n");
//                PyErr_Occurred();
//                return NULL;
//            }
//            else {
//                if (strcmp(temp_str_bytes, "none") == 0) {
//                    none = FALSE_m12;
//                } else if (strcmp(temp_str_bytes, "all") == 0) {
//                    all = TRUE_m12;
//                } else {
//                    sprintf(chan_name, "%s", temp_str_bytes);
//                }
//            }
//        } else {
//            PyErr_SetString(PyExc_RuntimeError, "chan_name (input 2) can be specified as a string\n");
//            PyErr_Occurred();
//            return NULL;
//        }
//    }
//
//    if (is_active_obj != NULL)
//    {
//        if (PyBool_Check(is_active_obj)) {
//            if (is_active_obj == Py_True)
//                is_active = TRUE_m12;
//            else
//                is_active = FALSE_m12;
//        } else {
//            PyErr_SetString(PyExc_RuntimeError, "is_active (input 3) can be specified as a boolean\n");
//            PyErr_Occurred();
//            return NULL;
//        }
//    }
//
//
//    seq = PyObject_GetIter(pointers_obj);
//    item=PyIter_Next(seq);
//    //globals_m12 = (GLOBALS_m12*) (PyLong_AsLongLong(item));
//    item=PyIter_Next(seq);
//    //globals_m12 = (GLOBALS_m12*) (PyLong_AsLongLong(item));
//    item=PyIter_Next(seq);
//    sess = (SESSION_m12*) (PyLong_AsLongLong(item));
//    //sess = change_pointer(sess, globals_m12);
//
//    strcpy(reference_chan_temp, globals_m12->reference_channel_name);
//
//    for (i = n_active_chans = 0; i < sess->number_of_time_series_channels; ++i) {
//        chan = sess->time_series_channels[i];
//
//        if ((strcmp(chan_name, chan->name) == 0) || (all == TRUE_m12)) {
//            if (is_active == TRUE_m12) {
//                chan->flags |= LH_CHANNEL_ACTIVE_m12;
//                //printf("Turning chan %s on\n", chan->name);
//            } else {
//                chan->flags &= ~LH_CHANNEL_ACTIVE_m12;
//                //printf("Turning chan %s off\n", chan->name);
//                //if (!strcmp(chan->name, reference_chan_temp))
//                //    printf("Warning: %s is the reference channel, and is now inactive. Please set a new reference channel, if reading by index values.\n", chan->name);
//            }
//        }
//    }
//
//    Py_INCREF(Py_None);
//    return Py_None;
//}

PyObject *get_channel_reference(PyObject *self, PyObject *args)
{
    return PyUnicode_FromString(globals_m12->reference_channel_name);
}

PyObject *set_channel_reference(PyObject *self, PyObject *args)
{
    SESSION_m12             *sess;
    PyObject                *sess_capsule_object, *chan_name_obj;
    si1                     chan_name[BASE_FILE_NAME_BYTES_m12];
    PyObject                *temp_UTF_str;
    si1                     *temp_str_bytes;

    
    chan_name_obj = NULL;
    
    // --- Parse the input ---
    if (!PyArg_ParseTuple(args,"OO",
                          &sess_capsule_object,
                          &chan_name_obj)){
        
        PyErr_SetString(PyExc_RuntimeError, "2 inputs required: pointers, chan_name\n");
        PyErr_Occurred();
        return NULL;
    }
        
    sess = (SESSION_m12*) PyCapsule_GetPointer(sess_capsule_object, "session");
    
    if (chan_name_obj != NULL)
    {
        if (PyUnicode_Check(chan_name_obj)) {
            temp_UTF_str = PyUnicode_AsEncodedString(chan_name_obj, "utf-8","strict"); // Encode to UTF-8 python objects
            temp_str_bytes = PyBytes_AS_STRING(temp_UTF_str); // Get the *char
            
            if (!*temp_str_bytes) {
                PyErr_SetString(PyExc_RuntimeError, "chan_name (input 2) can be specified as a string\n");
                PyErr_Occurred();
                return NULL;
            }
            else {
                sprintf(chan_name, "%s", temp_str_bytes);
            }
        } else {
            PyErr_SetString(PyExc_RuntimeError, "chan_name (input 2) can be specified as a string\n");
            PyErr_Occurred();
            return NULL;
        }
    }
    
    G_change_reference_channel_m12(sess, NULL, chan_name, DEFAULT_CHANNEL_m12);
    
    //printf("The reference channel is now set to: %s\n", globals_m12->reference_channel_name);
    
    Py_INCREF(Py_None);
    return Py_None;
}


PyObject *get_globals_number_of_session_samples(PyObject *self, PyObject *args)
{
    return PyLong_FromLongLong(globals_m12->number_of_session_samples);
}

PyObject *find_discontinuities(PyObject *self, PyObject *args)
{
    SESSION_m12             *sess;
    PyObject                *sess_capsule_object;
    si8                     i, num_contigua;
    CONTIGUON_m12           *contigua;
    PyObject                *py_contiguon, *py_contigua;

    // --- Parse the input ---
    if (!PyArg_ParseTuple(args,"O",
                          &sess_capsule_object)){
        
        PyErr_SetString(PyExc_RuntimeError, "2 inputs required: pointers\n");
        PyErr_Occurred();
        return NULL;
    }
        
    sess = (SESSION_m12*) PyCapsule_GetPointer(sess_capsule_object, "session");

    if (sess == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid session pointer\n");
        PyErr_Occurred();
        return NULL;
    }
    
    // find contiguous segments
    contigua = G_find_discontinuities_m12((LEVEL_HEADER_m12 *) sess, &num_contigua);
    
    py_contigua = PyList_New(num_contigua);
    
    for (i=0; i<num_contigua; ++i) {
        
        py_contiguon = Py_BuildValue("{s:L,s:L,s:L,s:L}",
                                     "start_index", contigua[i].start_sample_number,
                                     "end_index", contigua[i].end_sample_number,
                                     "start_time", contigua[i].start_time,
                                     "end_time", contigua[i].end_time);
        
        PyList_SetItem(py_contigua, i, py_contiguon);
    }
    
    free_m12(contigua, __FUNCTION__);

    return py_contigua;
}

PyObject *get_session_records(PyObject *self, PyObject *args)
{
    SESSION_m12             *sess;
    PyObject                *sess_capsule_object;
    si8                     start_time, end_time;
    TIME_SLICE_m12          local_sess_slice, *slice;
    ui8                     flags;
    PyObject                *start_time_input_obj, *end_time_input_obj;
    PyObject                *start_index_input_obj, *end_index_input_obj;
    si1                     *temp_str_bytes;
    PyObject                *temp_UTF_str;
    PyObject                *py_records;
    
    start_time_input_obj = NULL;
    end_time_input_obj = NULL;
    start_index_input_obj = Py_None;
    end_index_input_obj = Py_None;
    
    // set defaults for optional arguements
    start_time = UUTC_NO_ENTRY_m12;
    end_time = UUTC_NO_ENTRY_m12;
    
    // --- Parse the input ---
    if (!PyArg_ParseTuple(args,"OOO",
                          &sess_capsule_object,
                          &start_time_input_obj,
                          &end_time_input_obj)){
        
        PyErr_SetString(PyExc_RuntimeError, "3 inputs required: pointers, start_time, end_time\n");
        PyErr_Occurred();
        return NULL;
    }
        
    sess = (SEGMENT_m12*) PyCapsule_GetPointer(sess_capsule_object, "session");

    // read session to get records
    slice = &local_sess_slice;
    G_initialize_time_slice_m12(slice);
    initialize_time_slice(slice, start_index_input_obj, end_index_input_obj, start_time_input_obj, end_time_input_obj);
//    G_show_time_slice_m12(slice);
    
    // set flags to only get records
    //flags = LH_READ_SLICE_SESSION_RECORDS_m12 | LH_READ_SLICE_SEGMENTED_SESS_RECS_m12 | LH_MAP_ALL_SEGMENTS_m12;
    ui8 new_flags;
    new_flags = LH_READ_SLICE_ALL_RECORDS_m12 | LH_GENERATE_EPHEMERAL_DATA_m12;
    G_propogate_flags_m12((LEVEL_HEADER_m12 *) sess, new_flags);
    //G_read_session_m12(sess, sess_slice, NULL, 0, flags, NULL);
    G_read_session_m12(sess, slice);

    // session records
    py_records = fill_session_records(sess, NULL);
    
    return (py_records);
}
