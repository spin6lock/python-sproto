#include <Python.h>
#include "sproto.h"

typedef int bool;
#define true 1
#define false 0

static PyObject*
myfunc(PyObject *self, PyObject *args) {
    int a, b;
    if (!PyArg_ParseTuple(args,"ii", &a, &b)) {
        return Py_None;
    }
    return Py_BuildValue("i", a + b);
}

struct encode_ud {
    struct sproto_type *st;
    int list_index;
    PyObject *table;
	const char *tagname;
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
            printf("PyString:%s\n", string_ptr);
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
    self.list_index = -1;
    self.st = sprototype;
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

static PyMethodDef my_methods[] = {
    {"myadd", myfunc, METH_VARARGS},
    {"sproto_create", py_sproto_create, METH_VARARGS},
    {"sproto_type", py_sproto_type, METH_VARARGS},
    {"sproto_encode", py_sproto_encode, METH_VARARGS},
    {NULL, NULL}
};

PyMODINIT_FUNC
initexample(void){
    Py_InitModule("example", my_methods);
}

