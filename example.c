#include <Python.h>
#include "sproto.h"

typedef int bool;
#define true 1
#define false 0

struct encode_ud {
    PyObject *table;
};

static int encode(void *ud, const char *tagname, int type,\
        int index, struct sproto_type *st, void *value, int length) {
    printf("tagname:%s, type:%d, index:%d, length:%d\n", tagname, type, index, length);
    struct encode_ud *self = ud;
    PyObject *data = NULL;
    data = PyDict_GetItemString(self->table, tagname);
    if (data == NULL ) {
        printf("\tnot exist\n");
        return 0;
    }
    if (index > 0) {
        if (!PyList_Check(data)) {
            printf("data is not list\n");
            return 0;
        }
        int len = PyList_Size(data);
        if (index > len) {
            printf("data is finish\n");
            return 0;
        }
        data = PyList_GetItem(data, index - 1);
    } else {
        //do nothing 
    }
    //self->tagname = tagname;
    switch(type) {
        case SPROTO_TINTEGER: {
            printf("PyInt Check:%s\n", PyInt_Check(data)?"true":"false");
            //TODO error handling PyInt_AsLong(data) != -1
            long i = PyInt_AsLong(data);
            printf("int value:%lu\n", i);
            *(uint64_t *)value = (uint64_t)i;
            return 8;
        }
        case SPROTO_TBOOLEAN: {
            printf("PyBool Check:%s\n", PyBool_Check(data)?"true":"false");
            if (data == Py_True) {
                *(int*)value = 1;
            } else {
                *(int*)value = 0;
            }
            return 4;
        }
        case SPROTO_TSTRING: {
            //printf("PyString Check:%s\n", PyString_Check(data)?"true":"false");
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
    }
}

void 
sproto_free(PyObject *ptr) {
    struct sproto *sp = PyCapsule_GetPointer(ptr, NULL);
    if (sp != NULL) {
        sproto_release(sp);
    }
    return 0;
}

static PyObject*
py_sproto_create(PyObject *self, PyObject *args) {
    char *buffer;
    int sz = 0;
    struct sproto *sp;
    if (!PyArg_ParseTuple(args, "s#", &buffer, &sz)) {
        printf("create failed\n");
        return Py_None;
    }
    sp = sproto_create(buffer, sz);
    if (sp) {
        printf("capsule succ\n");
        return PyCapsule_New(sp, NULL, sproto_free);
    }
    return Py_None;
}

struct decode_ud {
    PyObject *table;
};

static int
decode(void *ud, const char *tagname, int type, int index, struct sproto_type *st, void *value, int length) {
    printf("tagname:%s, type:%d, index:%d, length:%d\n", tagname, type, index, length);
	struct decode_ud * self = ud;
    printf("table pointer: %p\n", (void*)self->table);
    switch(type) {
    case SPROTO_TINTEGER: {
        printf("set integer\n");
        PyDict_SetItemString(self->table, tagname, Py_BuildValue("I", *(uint64_t*)value));
        printf("table size:%d\n", PyDict_Size(self->table));
		break;
	}
	case SPROTO_TBOOLEAN: {
        printf("set bool\n");
        PyObject *bool_result = *(int*)value > 0 ? Py_True : Py_False;
        PyDict_SetItemString(self->table, tagname, bool_result);
        printf("table size:%d\n", PyDict_Size(self->table));
		break;
	}
	case SPROTO_TSTRING: {
        printf("set string\n");
        PyDict_SetItemString(self->table, tagname, Py_BuildValue("s#", (char*)value, length));
        printf("table size:%d\n", PyDict_Size(self->table));
		break;
	}
	case SPROTO_TSTRUCT: {
        printf("pack struct\n");
		struct decode_ud sub;
		int r;
        sub.table = PyDict_New();
        PyObject *data = PyDict_GetItemString(self->table, tagname);
        if (index > 0) {
            if (data == NULL) {
                PyObject *list = PyList_New(0);
                PyList_Append(list, sub.table);
                PyDict_SetItemString(self->table, tagname, list); //TODO check set item succ
            } else {
                PyList_Append(data, sub.table);
            }
        }
		r = sproto_decode(st, value, length, decode, &sub);
        printf("int r:%d\n", r);
		if (r < 0 || r != length) {
            printf("after decode:%d\n", r);
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
        printf("get proto failed\n");
        return Py_None;
    }
    struct sproto *sp = PyCapsule_GetPointer(sp_ptr,NULL);
    struct sproto_type *st = sproto_type(sp, name);
    if (st) {
        printf("succ query proto type\n");
        return PyCapsule_New(st, NULL, NULL);
    }
}

static PyObject*
py_sproto_encode(PyObject *pymodule, PyObject *args) {
    PyObject *st_capsule;
    char *sprototype;
    PyObject *table;
    if (!PyArg_ParseTuple(args, "OO", &st_capsule, &table)) {
        return Py_None;
    }
    sprototype = PyCapsule_GetPointer(st_capsule, NULL);
    int sz = 4096;
    void *buffer = PyMem_Malloc(sz);
    struct encode_ud self;
    self.table = table;
    for (;;) {
        int r = sproto_encode(sprototype, buffer, sz, encode, &self);
        if (r < 0) {
            buffer = PyMem_Realloc(buffer, sz * 2);
            sz = sz * 2;
        } else {
            return Py_BuildValue("s#", buffer, r);
        }
    }
}

static PyObject*
py_sproto_decode(PyObject *pymodule, PyObject *args) {
    PyObject *st_capsule;
    char *buffer;
    int sz = 0;
    char *sprototype;
    struct decode_ud self;
    self.table = PyDict_New();
    if (!PyArg_ParseTuple(args, "Os#", &st_capsule, &buffer, &sz)) {
        printf("decode para error");
        return Py_None;
    }
    printf("msg len:%d\n", sz);
    sprototype = PyCapsule_GetPointer(st_capsule, NULL);
    printf("original table size:%d\n", PyDict_Size(self.table));
    printf("original table addr:%p\n", (void*)self.table);
    int r = sproto_decode(sprototype, buffer, sz, decode, &self);
    if (r < 0) {
        printf("sproto c lib error\n");
        return Py_None;
    }
    //printf("table size:%d\n", PyDict_Size(self.table));
    //PyDict_SetItemString(self.table, "phone", Py_BuildValue("I", 10086));
    return Py_BuildValue("O", self.table);
}

static PyMethodDef my_methods[] = {
    {"sproto_create", py_sproto_create, METH_VARARGS},
    {"sproto_type", py_sproto_type, METH_VARARGS},
    {"sproto_encode", py_sproto_encode, METH_VARARGS},
    {"sproto_decode", py_sproto_decode, METH_VARARGS},
    {NULL, NULL}
};

PyMODINIT_FUNC
initexample(void){
    Py_InitModule("example", my_methods);
}

