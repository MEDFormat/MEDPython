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

#include <Python.h>

#include "medlib_m13.h"
#include "medrec_m13.h"

#define EPSILON 0.0001
#define FLOAT_EQUAL(x,y) ( ((y - EPSILON) < x) && (x <( y + EPSILON)) )
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

// Version
#define LS_READ_MED_PROD_VER_MAJOR        ((ui1) 1)
#define LS_READ_MED_PROD_VER_MINOR        ((ui1) 0)

/* Python methods definitions and help */

static char pymed_file_docstring[] =
    "This submodule provides a wrapper around Multiscale Electrophysiology Data (MED) version 1.0.1 library.";


static PyObject     *read_MED_exec(SESS_m13 *sess, si4 n_files, si8 start_time, si8 end_time, si8 start_idx, si8 end_idx, si1 *password, si1 *ref_chan, tern samples_as_singles);
PyObject*    fill_metadata(LH_m13 *level_header, SLICE_m13 *slice);
PyObject*   build_contigua(CHAN_m13 *chan, si8 start_time, si8 end_time, tern relative_days);
PyObject*    fill_session_records(SESS_m13 *sess,  DATA_MATRIX_m13 *dm);
PyObject*    fill_record(LH_m13 *lh, REC_HDR_m13 *rh, tern relative_days);
PyObject*    fill_record_matrix(LH_m13 *lh, REC_HDR_m13 *rh,  DATA_MATRIX_m13 *dm, tern relative_days);
si4     rec_compare(const void *a, const void *b);
//static PyObject     *get_raw_page_exec(SESS_m13 *sess, si8 start_time, si8 end_time, si8 n_out_samps, si1 *password, tern antialias, tern detrend, tern trace_ranges);
PyObject*    build_contigua_dm(LH_m13 *lh, DATA_MATRIX_m13 *dm, tern relative_days);



static char test_api_docstring[] =
    "Just a test.";

static char read_MED_docstring[] =
        "Function to read MED session information.\n\n\
         Parameters\n\
         ----------\n\
         Returns\n\
         -------\n\
         data: session dictionary\n\
            Contains metadata, channel list, records list, contigua list, and password_hints";


static PyObject *test_api(PyObject *self, PyObject *args);
void *initialize_time_slice(SLICE_m13 *slice, PyObject *start_index_obj, PyObject *end_index_obj, PyObject *start_time_obj, PyObject *end_time_obj);


void session_capsule_destructor(PyObject *capsule);
void dm_capsule_destructor(PyObject *capsule);
static PyObject *initialize_session(PyObject *self, PyObject *args);
static PyObject *initialize_data_matrix(PyObject *self, PyObject *args);
static PyObject *set_session_capsule_destructor (PyObject *self, PyObject *args);
static PyObject *set_data_matrix_capsule_destructor (PyObject *self, PyObject *args);
static PyObject *remove_capsule_destructor (PyObject *self, PyObject *args);

static PyObject *read_MED(PyObject *self, PyObject *args);
static PyObject *open_MED(PyObject *self, PyObject *args);
static PyObject *read_session_info(PyObject *self, PyObject *args);
//static PyObject *get_raw_page(PyObject *self, PyObject *args);
static PyObject *sort_channels_by_acq_num(PyObject *self, PyObject *args);
static PyObject *read_lh_flags(PyObject *self, PyObject *args);
static PyObject *push_lh_flags(PyObject *self, PyObject *args);
//static PyObject *set_single_channel_active(PyObject *self, PyObject *args);
static PyObject *get_channel_reference(PyObject *self, PyObject *args);
static PyObject *set_channel_reference(PyObject *self, PyObject *args);
static PyObject *get_globals_number_of_session_samples(PyObject *self, PyObject *args);
static PyObject *find_discontinuities(PyObject *self, PyObject *args);
static PyObject *get_session_records(PyObject *self, PyObject *args);


static PyObject *read_dm_flags(PyObject *self, PyObject *args);
static PyObject *push_dm_flags(PyObject *self, PyObject *args);
static PyObject *get_dm(PyObject *self, PyObject *args);


/* Specification of the members of the module */
static PyMethodDef module_methods[] = {
    {"test_api", test_api, METH_VARARGS, test_api_docstring},
    {"initialize_session", initialize_session, METH_VARARGS, read_MED_docstring},
    {"initialize_data_matrix", initialize_data_matrix, METH_VARARGS, read_MED_docstring},
    {"set_session_capsule_destructor", set_session_capsule_destructor, METH_VARARGS, read_MED_docstring},
    {"set_data_matrix_capsule_destructor", set_data_matrix_capsule_destructor, METH_VARARGS, read_MED_docstring},
    {"remove_capsule_destructor", remove_capsule_destructor, METH_VARARGS, read_MED_docstring},
    {"read_MED", read_MED, METH_VARARGS, read_MED_docstring},
    {"open_MED", open_MED, METH_VARARGS, read_MED_docstring},
    {"read_session_info", read_session_info, METH_VARARGS, read_MED_docstring},
//    {"get_raw_page", get_raw_page, METH_VARARGS, read_MED_docstring},
    {"sort_channels_by_acq_num", sort_channels_by_acq_num, METH_VARARGS, read_MED_docstring},
    {"read_lh_flags", read_lh_flags, METH_VARARGS, read_MED_docstring},
    {"push_lh_flags", push_lh_flags, METH_VARARGS, read_MED_docstring},
//    {"set_single_channel_active", set_single_channel_active, METH_VARARGS, read_MED_docstring},
    {"get_channel_reference", get_channel_reference, METH_VARARGS, read_MED_docstring},
    {"set_channel_reference", set_channel_reference, METH_VARARGS, read_MED_docstring},
    {"get_globals_number_of_session_samples", get_globals_number_of_session_samples, METH_VARARGS, read_MED_docstring},
    {"find_discontinuities", find_discontinuities, METH_VARARGS, read_MED_docstring},
    {"get_session_records", get_session_records, METH_VARARGS, read_MED_docstring},
    {"read_dm_flags", read_dm_flags, METH_VARARGS, read_MED_docstring},
    {"push_dm_flags", push_dm_flags, METH_VARARGS, read_MED_docstring},
    {"get_dm", get_dm, METH_VARARGS, read_MED_docstring},
    {NULL, NULL, 0, NULL}
};

/* Definition of struct for python 3 */
static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "dhn_med.dhnmed_file.dhnmed_file",     /* m_name */
    pymed_file_docstring,  /* m_doc */
    -1,                  /* m_size */
    module_methods,    /* m_methods */
    NULL,                /* m_reload */
    NULL,                /* m_traverse */
    NULL,                /* m_clear */
    NULL,                /* m_free */
};

/* Module initialisation */
PyObject * PyInit_dhnmed_file(void)
{
    PyObject *m = PyModule_Create(&moduledef);

    if (m == NULL)
        return NULL;

    return m;
}
