#include <Python.h>
#include "sproto.h"

typedef int bool;
#define true 1
#define false 0

static PyObject *SprotoError;

struct encode_ud {
    PyObject *table;
};

static int encode(void *ud, const char *tagname, int type,\
        int index, struct sproto_type *st, void *value, int length) {
    //printf("tagname:%s, type:%d, index:%d, length:%d\n", tagname, type, index, length);
    struct encode_ud *self = ud;
    PyObject *data = NULL;
    data = PyDict_GetItemString(self->table, tagname);
    if (data == NULL ) {
        //printf("\tnot exist\n");
        return 0;
    }
    if (index > 0) {
        if (!PyList_Check(data)) {
            //printf("data is not list\n");
            return 0;
        }
        int len = PyList_Size(data);
        if (index > len) {
            //printf("data is finish\n");
            return 0;
        }
        data = PyList_GetItem(data, index - 1);
    }
    //self->tagname = tagname;
    switch(type) {
        case SPROTO_TINTEGER: {
            if (!PyInt_Check(data)) {
                PyErr_SetObject(SprotoError, PyString_FromFormat("type mismatch, tag:%s, expected int", tagname));
                return -1;
            }
            long i = PyInt_AsLong(data);
            int vh = i >> 31;
            if (vh == 0 || vh == -1) {
                *(uint32_t *)value = (uint32_t)i;
                return 4;
            } else {
                *(uint64_t *)value = (uint64_t)i;
                return 8;
            }
        }
        case SPROTO_TBOOLEAN: {
            //printf("PyBool Check:%s\n", PyBool_Check(data)?"true":"false");
            if (data == Py_True) {
                *(int*)value = 1;
            } else {
                *(int*)value = 0;
            }
            return 4;
        }
        case SPROTO_TSTRING: {
            //printf("PyString Check:%s\n", PyString_Check(data)?"true":"false");
            if (!PyString_Check(data)) {
                PyErr_SetObject(SprotoError, PyString_FromFormat("type mismatch, tag:%s, expected string", tagname));
                return -1;
            }

            char* string_ptr = NULL;
            Py_ssize_t len = 0;
            PyString_AsStringAndSize(data, &string_ptr, &len);
            memcpy(value, string_ptr, (size_t)len);
            return len;
        }
        case SPROTO_TSTRUCT: {
            struct encode_ud sub;
            sub.table = data;
            return sproto_encode(st, value, length, encode, &sub);
        }
        default:
            return 0;
    }
}

void 
sproto_free(PyObject *ptr) {
    struct sproto *sp = PyCapsule_GetPointer(ptr, NULL);
    if (sp != NULL) {
        sproto_release(sp);
    }
}

static PyObject*
py_sproto_create(PyObject *self, PyObject *args) {
    char *buffer;
    int sz = 0;
    struct sproto *sp;
    if (!PyArg_ParseTuple(args, "s#", &buffer, &sz)) {
        return NULL;
    }
    sp = sproto_create(buffer, sz);
    if (sp) {
        //printf("capsule succ\n");
        return PyCapsule_New(sp, NULL, sproto_free);
    }
    return Py_None;
}

struct decode_ud {
    PyObject *table;
};

static int
decode(void *ud, const char *tagname, int type, int index, struct sproto_type *st, void *value, int length) {
    //printf("tagname:%s, type:%d, index:%d, length:%d\n", tagname, type, index, length);
	struct decode_ud * self = ud;
    //printf("table pointer: %p\n", (void*)self->table);
    PyObject *obj = self->table;
    if (index > 0) {
        obj = PyDict_GetItemString(self->table, tagname);
        if (obj == NULL) {
            PyObject *list = PyList_New(0);
            PyDict_SetItemString(self->table, tagname, list);
            obj = list;
        }
    }
    switch(type) {
    case SPROTO_TINTEGER: {
        //printf("set integer\n");
        PyObject *data = Py_BuildValue("i", *(uint64_t*)value);
        if (PyList_Check(obj)) {
            PyList_Append(obj, data);
        } else {
            PyDict_SetItemString(self->table, tagname, data);
        }
        //printf("table size:%d\n", PyDict_Size(self->table));
		break;
	}
	case SPROTO_TBOOLEAN: {
        //printf("set bool\n");
        PyObject *data = *(int*)value > 0 ? Py_True : Py_False;
        if (PyList_Check(obj)) {
            PyList_Append(obj, data);
        } else {
            PyDict_SetItemString(self->table, tagname, data);
        }
        //printf("table size:%d\n", PyDict_Size(self->table));
		break;
	}
	case SPROTO_TSTRING: {
        //printf("set string\n");
        PyObject *data = Py_BuildValue("s#", (char*)value, length);
        if (PyList_Check(obj)) {
            PyList_Append(obj, data);
        } else {
            PyDict_SetItemString(self->table, tagname, data);
        }
        //printf("table size:%d\n", PyDict_Size(self->table));
		break;
	}
	case SPROTO_TSTRUCT: {
        //printf("pack struct\n");
		struct decode_ud sub;
		int r;
        sub.table = PyDict_New();
        if (PyList_Check(obj)) {
            PyList_Append(obj, sub.table);
        } else {
            PyDict_SetItemString(self->table, tagname, sub.table);
        }
		r = sproto_decode(st, value, length, decode, &sub);
        //printf("int r:%d\n", r);
		if (r < 0 || r != length) {
            //printf("after decode:%d\n", r);
			return r;
        }
		break;
	}
    }
    return 0;
}

static PyObject*
py_sproto_type(PyObject *self, PyObject *args) {
    PyObject *sp_ptr;
    char *name;
    if (!PyArg_ParseTuple(args, "Os", &sp_ptr, &name)) {
        return NULL;
    }
    struct sproto *sp = PyCapsule_GetPointer(sp_ptr,NULL);
    struct sproto_type *st = sproto_type(sp, name);
    if (st) {
        return PyCapsule_New(st, NULL, NULL);
    }
    return Py_None;
}

static PyObject*
py_sproto_encode(PyObject *pymodule, PyObject *args) {
    PyObject *st_capsule;
    struct sproto_type *sprototype;
    PyObject *table;
    if (!PyArg_ParseTuple(args, "OO", &st_capsule, &table)) {
        return NULL;
    }
    sprototype = PyCapsule_GetPointer(st_capsule, NULL);
    int sz = 4096;
    void *buffer = PyMem_Malloc(sz);
    struct encode_ud self;
    self.table = table;
    for (;;) {
        int r = sproto_encode(sprototype, buffer, sz, encode, &self);
        if (r < 0) {
            if (PyErr_Occurred()) {
                PyMem_Free(buffer);
                return NULL;
            }
            buffer = PyMem_Realloc(buffer, sz * 2);
            sz = sz * 2;
        } else {
            PyObject *t = Py_BuildValue("s#", buffer, r);
            PyMem_Free(buffer);
            return t;
        }
    }
}

static PyObject*
py_sproto_decode(PyObject *pymodule, PyObject *args) {
    PyObject *st_capsule;
    char *buffer;
    int sz = 0;
    struct sproto_type *sprototype;
    struct decode_ud self;
    self.table = PyDict_New();
    if (!PyArg_ParseTuple(args, "Os#", &st_capsule, &buffer, &sz)) {
        return NULL;
    }
    //printf("msg len:%d\n", sz);
    sprototype = PyCapsule_GetPointer(st_capsule, NULL);
    int r = sproto_decode(sprototype, buffer, sz, decode, &self);
    if (r < 0) {
        PyErr_SetString(SprotoError, "sproto c lib error");
        return NULL;
    }
    //printf("table size:%d\n", PyDict_Size(self.table));
    return Py_BuildValue("O", self.table);
}

static PyObject*
py_sproto_pack(PyObject *pymodule, PyObject *args) {
    char *srcbuffer;
    int srcsz;
    if (!PyArg_ParseTuple(args, "s#", &srcbuffer, &srcsz)) {
        return NULL;
    }
    int maxsz = (srcsz + 2047) / 2048 * 2 + srcsz;
    void *dstbuffer = PyMem_Malloc(maxsz);
    int bytes = sproto_pack(srcbuffer, srcsz, dstbuffer, maxsz);
    if (bytes > maxsz) {
        return Py_None;
    }
    PyObject *t = Py_BuildValue("s#", dstbuffer, bytes);
    PyMem_Free(dstbuffer);
    return t; 
}

static PyObject*
py_sproto_unpack(PyObject *pymodule, PyObject *args) {
    char *srcbuffer;
    int srcsz;
    if (!PyArg_ParseTuple(args, "s#", &srcbuffer, &srcsz)) {
        return NULL;
    }
    int dstsz = 1024;
    void *dstbuffer = PyMem_Malloc(dstsz);
    int r = sproto_unpack(srcbuffer, srcsz, dstbuffer, dstsz);
    if (r < 0) {
        PyMem_Free(dstbuffer);
        return Py_None;
    }
    if (r > dstsz) {
        dstbuffer = PyMem_Realloc(dstbuffer, r);
    }
    r = sproto_unpack(srcbuffer, srcsz, dstbuffer, dstsz);
    if (r < 0) {
        PyMem_Free(dstbuffer);
        return Py_None;
    }
    PyObject *t = Py_BuildValue("s#", dstbuffer, r);
    PyMem_Free(dstbuffer);
    return t;
}

static PyObject*
py_sproto_protocol(PyObject *pymodule, PyObject *args) {
    PyObject *sp_ptr;
    PyObject *name_or_tagid;
    if (!PyArg_ParseTuple(args, "OO", &sp_ptr, &name_or_tagid)) {
        return NULL;
    }
    struct sproto *sp = PyCapsule_GetPointer(sp_ptr, NULL);
    int tagid;
    const char *name;
    PyObject *ret;
    if (PyInt_Check(name_or_tagid)) {
        tagid = PyInt_AsLong(name_or_tagid);
        if (PyErr_Occurred()) {
            return NULL;
        }
        name = sproto_protoname(sp, tagid);
        ret = Py_BuildValue("s", name);
    } else {
        name = PyString_AsString(name_or_tagid);
        tagid = sproto_prototag(sp, name);
        ret = Py_BuildValue("i", tagid);
    }
    struct sproto_type *request;
    struct sproto_type *response;
    PyObject *py_request;
    PyObject *py_response;
    request = sproto_protoquery(sp, tagid, SPROTO_REQUEST);
    py_request = request == NULL ? Py_None:PyCapsule_New(request, NULL, NULL);
    response = sproto_protoquery(sp, tagid, SPROTO_REQUEST);
    py_response = response == NULL? Py_None:PyCapsule_New(response, NULL, NULL);
    return Py_BuildValue("OOO", ret, py_request, py_response);
}

static PyMethodDef pysproto_methods[] = {
    {"sproto_create", py_sproto_create, METH_VARARGS},
    {"sproto_type", py_sproto_type, METH_VARARGS},
    {"sproto_encode", py_sproto_encode, METH_VARARGS},
    {"sproto_decode", py_sproto_decode, METH_VARARGS},
    {"sproto_pack", py_sproto_pack, METH_VARARGS},
    {"sproto_unpack", py_sproto_unpack, METH_VARARGS},
    {"sproto_protocol", py_sproto_protocol, METH_VARARGS},
    {NULL, NULL}
};

PyMODINIT_FUNC
initpysproto(void){
    PyObject *m;
    m = Py_InitModule("pysproto", pysproto_methods);
    if (m == NULL) {
        return;
    }
    SprotoError = PyErr_NewException("pysproto.error", NULL, NULL);
    Py_INCREF(SprotoError);
    PyModule_AddObject(m, "error", SprotoError);
}

