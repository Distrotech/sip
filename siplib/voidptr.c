/*
 * SIP library code.
 *
 * Copyright (c) 2010 Riverbank Computing Limited <info@riverbankcomputing.com>
 *
 * This file is part of SIP.
 *
 * This copy of SIP is licensed for use under the terms of the SIP License
 * Agreement.  See the file LICENSE for more details.
 *
 * This copy of SIP may also used under the terms of the GNU General Public
 * License v2 or v3 as published by the Free Software Foundation which can be
 * found in the files LICENSE-GPL2 and LICENSE-GPL3 included in this package.
 *
 * SIP is supplied WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */


#include <Python.h>

#include <stddef.h>

#include "sip.h"
#include "sipint.h"


/* The object data structure. */
typedef struct {
    PyObject_HEAD
    void *voidptr;
    SIP_SSIZE_T size;
    int rw;
} sipVoidPtrObject;


/* The structure used to hold the results of a voidptr conversion. */
struct vp_values {
    void *voidptr;
    SIP_SSIZE_T size;
    int rw;
};


static int check_size(PyObject *self);
static PyObject *make_voidptr(void *voidptr, SIP_SSIZE_T size, int rw);
static int vp_convertor(PyObject *arg, struct vp_values *vp);


#if defined(SIP_USE_PYCAPSULE)
/*
 * Implement ascapsule() for the type.
 */
static PyObject *sipVoidPtr_ascapsule(sipVoidPtrObject *v, PyObject *arg)
{
    return PyCapsule_New(v->voidptr, NULL, NULL);
}
#endif


#if defined(SIP_SUPPORT_PYCOBJECT)
/*
 * Implement ascobject() for the type.
 */
static PyObject *sipVoidPtr_ascobject(sipVoidPtrObject *v, PyObject *arg)
{
    return PyCObject_FromVoidPtr(v->voidptr, NULL);
}
#endif


/*
 * Implement asstring() for the type.
 */
static PyObject *sipVoidPtr_asstring(sipVoidPtrObject *v, PyObject *args,
        PyObject *kw)
{
    static char *kwlist[] = {"size", NULL};

    SIP_SSIZE_T size = -1;

    if (!PyArg_ParseTupleAndKeywords(args, kw,
#if PY_VERSION_HEX >= 0x02050000
            "|n:asstring",
#else
            "|i:asstring",
#endif
            kwlist, &size))
        return NULL;

    /* Use the current size if one wasn't explicitly given. */
    if (size < 0)
        size = v->size;

    if (size < 0)
    {
        PyErr_SetString(PyExc_ValueError,
                "a size must be given or the sip.voidptr must have a size");
        return NULL;
    }

    return SIPBytes_FromStringAndSize(v->voidptr, size);
}


/*
 * Implement getsize() for the type.
 */
static PyObject *sipVoidPtr_getsize(sipVoidPtrObject *v, PyObject *arg)
{
#if PY_MAJOR_VERSION >= 3
    return PyLong_FromSsize_t(v->size);
#elif PY_VERSION_HEX >= 0x02050000
    return PyInt_FromSsize_t(v->size);
#else
    return PyInt_FromLong(v->size);
#endif
}


/*
 * Implement setsize() for the type.
 */
static PyObject *sipVoidPtr_setsize(sipVoidPtrObject *v, PyObject *arg)
{
    SIP_SSIZE_T size;

#if PY_MAJOR_VERSION >= 3
    size = PyLong_AsSsize_t(arg);
#elif PY_VERSION_HEX >= 0x02050000
    size = PyInt_AsSsize_t(arg);
#else
    size = (int)PyInt_AsLong(arg);
#endif

    if (PyErr_Occurred())
        return NULL;

    v->size = size;

    Py_INCREF(Py_None);
    return Py_None;
}


/*
 * Implement getwriteable() for the type.
 */
static PyObject *sipVoidPtr_getwriteable(sipVoidPtrObject *v, PyObject *arg)
{
    return PyBool_FromLong(v->rw);
}


/*
 * Implement setwriteable() for the type.
 */
static PyObject *sipVoidPtr_setwriteable(sipVoidPtrObject *v, PyObject *arg)
{
    int rw;

    rw = (int)SIPLong_AsLong(arg);

    if (PyErr_Occurred())
        return NULL;

    v->rw = rw;

    Py_INCREF(Py_None);
    return Py_None;
}


/* The methods data structure. */
static PyMethodDef sipVoidPtr_Methods[] = {
#if defined(SIP_USE_PYCAPSULE)
    {"ascapsule", (PyCFunction)sipVoidPtr_ascapsule, METH_NOARGS, NULL},
#endif
#if defined(SIP_SUPPORT_PYCOBJECT)
    {"ascobject", (PyCFunction)sipVoidPtr_ascobject, METH_NOARGS, NULL},
#endif
    {"asstring", (PyCFunction)sipVoidPtr_asstring, METH_KEYWORDS, NULL},
    {"getsize", (PyCFunction)sipVoidPtr_getsize, METH_NOARGS, NULL},
    {"setsize", (PyCFunction)sipVoidPtr_setsize, METH_O, NULL},
    {"getwriteable", (PyCFunction)sipVoidPtr_getwriteable, METH_NOARGS, NULL},
    {"setwriteable", (PyCFunction)sipVoidPtr_setwriteable, METH_O, NULL},
    {NULL}
};


/*
 * Implement int() for the type.
 */
static PyObject *sipVoidPtr_int(PyObject *self)
{
    return PyLong_FromVoidPtr(((sipVoidPtrObject *)self)->voidptr);
}


#if PY_MAJOR_VERSION < 3
/*
 * Implement hex() for the type.
 */
static PyObject *sipVoidPtr_hex(PyObject *self)
{
    char buf[2 + 16 + 1];

    PyOS_snprintf(buf, sizeof (buf), "0x%.*lx", (int)(sizeof (void *) * 2),
            (unsigned long)((sipVoidPtrObject *)self)->voidptr);

    return PyString_FromString(buf);
}
#endif


/* The number methods data structure. */
static PyNumberMethods sipVoidPtr_NumberMethods = {
    0,                      /* nb_add */
    0,                      /* nb_subtract */
    0,                      /* nb_multiply */
#if PY_MAJOR_VERSION < 3
    0,                      /* nb_divide */
#endif
    0,                      /* nb_remainder */
    0,                      /* nb_divmod */
    0,                      /* nb_power */
    0,                      /* nb_negative */
    0,                      /* nb_positive */
    0,                      /* nb_absolute */
    0,                      /* nb_bool (Python v3), nb_nonzero (Python v2) */
    0,                      /* nb_invert */
    0,                      /* nb_lshift */
    0,                      /* nb_rshift */
    0,                      /* nb_and */
    0,                      /* nb_xor */
    0,                      /* nb_or */
#if PY_MAJOR_VERSION < 3
    0,                      /* nb_coerce */
#endif
    sipVoidPtr_int,         /* nb_int */
    0,                      /* nb_reserved (Python v3), nb_long (Python v2) */
    0,                      /* nb_float */
#if PY_MAJOR_VERSION < 3
    0,                      /* nb_oct */
    sipVoidPtr_hex,         /* nb_hex */
#endif
    0,                      /* nb_inplace_add */
    0,                      /* nb_inplace_subtract */
    0,                      /* nb_inplace_multiply */
#if PY_MAJOR_VERSION < 3
    0,                      /* nb_inplace_divide */
#endif
    0,                      /* nb_inplace_remainder */
    0,                      /* nb_inplace_power */
    0,                      /* nb_inplace_lshift */
    0,                      /* nb_inplace_rshift */
    0,                      /* nb_inplace_and */
    0,                      /* nb_inplace_xor */
    0,                      /* nb_inplace_or */
    0,                      /* nb_floor_divide */
    0,                      /* nb_true_divide */
    0,                      /* nb_inplace_floor_divide */
    0,                      /* nb_inplace_true_divide */
#if PY_VERSION_HEX >= 0x02050000
    0                       /* nb_index */
#endif
};


/*
 * Implement len() for the type.
 */
static SIP_SSIZE_T sipVoidPtr_length(PyObject *self)
{
    if (check_size(self) < 0)
        return -1;

    return ((sipVoidPtrObject *)self)->size;
}


/*
 * Implement sequence item sub-script for the type.
 */
static PyObject *sipVoidPtr_item(PyObject *self, SIP_SSIZE_T idx)
{
    if (check_size(self) < 0)
        return NULL;

    if (idx < 0 || idx >= ((sipVoidPtrObject *)self)->size)
    {
        PyErr_SetString(PyExc_IndexError, "voidptr index out of range");
        return NULL;
    }

    return SIPBytes_FromStringAndSize(
            (char *)((sipVoidPtrObject *)self)->voidptr + idx, 1);
}


#if PY_VERSION_HEX < 0x02050000
/*
 * Implement sequence slice sub-script for the type.
 */
static PyObject *sipVoidPtr_slice(PyObject *self, int left, int right)
{
    sipVoidPtrObject *v;

    if (check_size(self) < 0)
        return NULL;

    v = (sipVoidPtrObject *)self;

    /* Allow for bounds relative to the end of the data. */
    if (left < 0)
        left += v->size;

    if (right < 0)
        right += v->size;

    /*
     * If the bounds aren't valid then make sure we return a zero length
     * pointer.
     */
    if (left < 0 || right < 0 || left >= right || left >= v->size || right >= v->size)
        left = right = 0;

    return make_voidptr(v->voidptr + left, right - left, v->rw);
}
#endif


/* The sequence methods data structure. */
static PySequenceMethods sipVoidPtr_SequenceMethods = {
    sipVoidPtr_length,      /* sq_length */
    0,                      /* sq_concat */
    0,                      /* sq_repeat */
    sipVoidPtr_item,        /* sq_item */
#if PY_VERSION_HEX < 0x02050000
    sipVoidPtr_slice,       /* sq_slice */
#endif
};


#if PY_VERSION_HEX >= 0x02050000
/*
 * Implement mapping sub-script for the type.
 */
static PyObject *sipVoidPtr_subscript(PyObject *self, PyObject *key)
{
    sipVoidPtrObject *v;

    if (check_size(self) < 0)
        return NULL;

    v = (sipVoidPtrObject *)self;

    if (PyIndex_Check(key))
    {
        Py_ssize_t idx = PyNumber_AsSsize_t(key, PyExc_IndexError);

        if (idx == -1 && PyErr_Occurred())
            return NULL;

        if (idx < 0)
            idx += v->size;

        return sipVoidPtr_item(self, idx);
    }

    if (PySlice_Check(key))
    {
        Py_ssize_t start, stop, step, slicelength;

        if (PySlice_GetIndicesEx((PySliceObject *)key, v->size, &start, &stop, &step, &slicelength) < 0)
            return NULL;

        if (step != 1)
        {
            PyErr_SetString(PyExc_IndexError, "slice step must be 1");
            return NULL;
        }

        return make_voidptr(v->voidptr + start, slicelength, v->rw);
    }

    PyErr_Format(PyExc_TypeError, "cannot index memory using \"%s\"",
            key->ob_type->tp_name);

    return NULL;
}


/* The mapping methods data structure. */
static PyMappingMethods sipVoidPtr_MappingMethods = {
    sipVoidPtr_length,      /* mp_length */
    sipVoidPtr_subscript,   /* mp_subscript */
    0,                      /* mp_ass_subscript */
};
#endif


#if PY_VERSION_HEX >= 0x02060000
/*
 * The buffer implementation for Python v2.6 and later.
 */
static int sipVoidPtr_getbuffer(PyObject *self, Py_buffer *buf, int flags)
{
    sipVoidPtrObject *v;

    if (check_size(self) < 0)
        return -1;

    v = (sipVoidPtrObject *)self;

    return PyBuffer_FillInfo(buf, self, v->voidptr, v->size, !v->rw, flags);
}
#endif


#if PY_MAJOR_VERSION < 3
/*
 * The read buffer implementation for Python v2.
 */
static SIP_SSIZE_T sipVoidPtr_getreadbuffer(PyObject *self, SIP_SSIZE_T seg,
        void **ptr)
{
    sipVoidPtrObject *v;

    if (seg != 0)
    {
        PyErr_SetString(PyExc_SystemError, "invalid buffer segment");
        return -1;
    }

    if (check_size(self) < 0)
        return -1;

    v = (sipVoidPtrObject *)self;

    *ptr = v->voidptr;

    return v->size;
}
#endif


#if PY_MAJOR_VERSION < 3
/*
 * The write buffer implementation for Python v2.
 */
static SIP_SSIZE_T sipVoidPtr_getwritebuffer(PyObject *self, SIP_SSIZE_T seg,
        void **ptr)
{
    if (((sipVoidPtrObject *)self)->rw)
        return sipVoidPtr_getreadbuffer(self, seg, ptr);

    PyErr_SetString(PyExc_TypeError, "voidptr is not writeable");
    return -1;
}
#endif


#if PY_MAJOR_VERSION < 3
/*
 * The segment count implementation for Python v2.
 */
static SIP_SSIZE_T sipVoidPtr_getsegcount(PyObject *self, SIP_SSIZE_T *lenp)
{
    SIP_SSIZE_T segs, len;

    len = ((sipVoidPtrObject *)self)->size;
    segs = (len < 0 ? 0 : 1);

    if (lenp != NULL)
        *lenp = len;

    return segs;
}
#endif


/* The buffer methods data structure. */
static PyBufferProcs sipVoidPtr_BufferProcs = {
#if PY_MAJOR_VERSION >= 3
    sipVoidPtr_getbuffer,   /* bf_getbuffer */
    0                       /* bf_releasebuffer */
#else
    sipVoidPtr_getreadbuffer,   /* bf_getreadbuffer */
    sipVoidPtr_getwritebuffer,  /* bf_getwritebuffer */
    sipVoidPtr_getsegcount, /* bf_getsegcount */
#if PY_VERSION_HEX >= 0x02050000
    sipVoidPtr_getreadbuffer,   /* bf_getcharbuffer */
#if PY_VERSION_HEX >= 0x02060000
    sipVoidPtr_getbuffer,   /* bf_getbuffer */
    0                       /* bf_releasebuffer */
#endif
#else
    (getcharbufferproc)sipVoidPtr_getreadbuffer /* bf_getcharbuffer */
#endif
#endif
};


/*
 * Implement __new__ for the type.
 */
static PyObject *sipVoidPtr_new(PyTypeObject *subtype, PyObject *args,
        PyObject *kw)
{
    static char *kwlist[] = {"address", "size", "writeable", NULL};

    struct vp_values vp_conversion;
    SIP_SSIZE_T size = -1;
    int rw = -1;
    PyObject *obj;

    if (!PyArg_ParseTupleAndKeywords(args, kw,
#if PY_VERSION_HEX >= 0x02050000
            "O&|ni:voidptr",
#else
            "O&|ii:voidptr",
#endif
            kwlist, vp_convertor, &vp_conversion, &size, &rw))
        return NULL;

    /* Use the explicit size if one was given. */
    if (size >= 0)
        vp_conversion.size = size;

    /* Use the explicit writeable flag if one was given. */
    if (rw >= 0)
        vp_conversion.rw = rw;

    /* Create the instance. */
    if ((obj = subtype->tp_alloc(subtype, 0)) == NULL)
        return NULL;

    /* Save the values. */
    ((sipVoidPtrObject *)obj)->voidptr = vp_conversion.voidptr;
    ((sipVoidPtrObject *)obj)->size = vp_conversion.size;
    ((sipVoidPtrObject *)obj)->rw = vp_conversion.rw;

    return obj;
}


/* The type data structure. */
PyTypeObject sipVoidPtr_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "sip.voidptr",          /* tp_name */
    sizeof (sipVoidPtrObject),  /* tp_basicsize */
    0,                      /* tp_itemsize */
    0,                      /* tp_dealloc */
    0,                      /* tp_print */
    0,                      /* tp_getattr */
    0,                      /* tp_setattr */
    0,                      /* tp_reserved (Python v3), tp_compare (Python v2) */
    0,                      /* tp_repr */
    &sipVoidPtr_NumberMethods,  /* tp_as_number */
    &sipVoidPtr_SequenceMethods,    /* tp_as_sequence */
#if PY_VERSION_HEX >= 0x02050000
    &sipVoidPtr_MappingMethods, /* tp_as_mapping */
#else
    0,                      /* tp_as_mapping */
#endif
    0,                      /* tp_hash */
    0,                      /* tp_call */
    0,                      /* tp_str */
    0,                      /* tp_getattro */
    0,                      /* tp_setattro */
    &sipVoidPtr_BufferProcs,    /* tp_as_buffer */
#if defined(Py_TPFLAGS_HAVE_NEWBUFFER)
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_NEWBUFFER,   /* tp_flags */
#else
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /* tp_flags */
#endif
    0,                      /* tp_doc */
    0,                      /* tp_traverse */
    0,                      /* tp_clear */
    0,                      /* tp_richcompare */
    0,                      /* tp_weaklistoffset */
    0,                      /* tp_iter */
    0,                      /* tp_iternext */
    sipVoidPtr_Methods,     /* tp_methods */
    0,                      /* tp_members */
    0,                      /* tp_getset */
    0,                      /* tp_base */
    0,                      /* tp_dict */
    0,                      /* tp_descr_get */
    0,                      /* tp_descr_set */
    0,                      /* tp_dictoffset */
    0,                      /* tp_init */
    0,                      /* tp_alloc */
    sipVoidPtr_new,         /* tp_new */
};


/*
 * A convenience function to convert a C/C++ void pointer from a Python object.
 */
void *sip_api_convert_to_void_ptr(PyObject *obj)
{
    if (obj == NULL)
    {
        PyErr_SetString(PyExc_TypeError, "sip.voidptr is NULL");
        return NULL;
    }

    if (obj == Py_None)
        return NULL;

    if (PyObject_TypeCheck(obj, &sipVoidPtr_Type))
        return ((sipVoidPtrObject *)obj)->voidptr;

#if defined(SIP_USE_PYCAPSULE)
    if (PyCapsule_CheckExact(obj))
        return PyCapsule_GetPointer(obj, NULL);
#endif

#if defined(SIP_SUPPORT_PYCOBJECT)
    if (PyCObject_Check(obj))
        return PyCObject_AsVoidPtr(obj);
#endif

#if PY_MAJOR_VERSION >= 3
    return PyLong_AsVoidPtr(obj);
#else
    return (void *)PyInt_AsLong(obj);
#endif
}


/*
 * Convert a C/C++ void pointer to a sip.voidptr object.
 */
PyObject *sip_api_convert_from_void_ptr(void *val)
{
    return make_voidptr(val, -1, TRUE);
}


/*
 * Convert a C/C++ void pointer to a sip.voidptr object.
 */
PyObject *sip_api_convert_from_const_void_ptr(const void *val)
{
    return make_voidptr((void *)val, -1, FALSE);
}


/*
 * Convert a sized C/C++ void pointer to a sip.voidptr object.
 */
PyObject *sip_api_convert_from_void_ptr_and_size(void *val, SIP_SSIZE_T size)
{
    return make_voidptr(val, size, TRUE);
}


/*
 * Convert a sized C/C++ const void pointer to a sip.voidptr object.
 */
PyObject *sip_api_convert_from_const_void_ptr_and_size(const void *val,
        SIP_SSIZE_T size)
{
    return make_voidptr((void *)val, size, FALSE);
}


/*
 * Check that a void pointer has an explicit size and raise an exception if it
 * hasn't.
 */
static int check_size(PyObject *self)
{
    if (((sipVoidPtrObject *)self)->size >= 0)
        return 0;

    PyErr_SetString(PyExc_IndexError, "voidptr has an unknown size");

    return -1;
}


/*
 * Do the work of converting a void pointer.
 */
static PyObject *make_voidptr(void *voidptr, SIP_SSIZE_T size, int rw)
{
    sipVoidPtrObject *self;

    if (voidptr == NULL)
    {
        Py_INCREF(Py_None);
        return Py_None;
    }

    if ((self = PyObject_NEW(sipVoidPtrObject, &sipVoidPtr_Type)) == NULL)
        return NULL;

    self->voidptr = voidptr;
    self->size = size;
    self->rw = rw;

    return (PyObject *)self;
}


/*
 * Convert a Python object to the values needed to create a voidptr.
 */
static int vp_convertor(PyObject *arg, struct vp_values *vp)
{
    void *ptr;
    SIP_SSIZE_T size = -1;
    int rw = TRUE;

    if (arg == Py_None)
        ptr = NULL;
#if defined(SIP_USE_PYCAPSULE)
    else if (PyCapsule_CheckExact(arg))
        ptr = PyCapsule_GetPointer(arg, NULL);
#endif
#if defined(SIP_SUPPORT_PYCOBJECT)
    else if (PyCObject_Check(arg))
        ptr = PyCObject_AsVoidPtr(arg);
#endif
    else if (PyObject_TypeCheck(arg, &sipVoidPtr_Type))
    {
        ptr = ((sipVoidPtrObject *)arg)->voidptr;
        size = ((sipVoidPtrObject *)arg)->size;
        rw = ((sipVoidPtrObject *)arg)->rw;
    }
    else
    {
#if PY_MAJOR_VERSION >= 3
        ptr = PyLong_AsVoidPtr(arg);
#else
        ptr = (void *)PyInt_AsLong(arg);
#endif

        if (PyErr_Occurred())
        {
#if PY_VERSION_HEX >= 0x03010000
            PyErr_SetString(PyExc_TypeError, "a single integer, CObject, None or another voidptr is required");
#else
            PyErr_SetString(PyExc_TypeError, "a single integer, Capsule, CObject, None or another voidptr is required");
#endif
            return 0;
        }
    }

    vp->voidptr = ptr;
    vp->size = size;
    vp->rw = rw;

    return 1;
}
