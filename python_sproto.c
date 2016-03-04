#include <Python.h>
#include "sproto.h"

#ifdef _WIN32
#include "msvcint.h"
#endif

typedef int bool;
#define true 1
#define false 0

static PyObject *SprotoError;

struct encode_ud {
    PyObject *table;
    PyObject *values;
};

static int
encode(const struct sproto_arg *args) {
    struct encode_ud *self = args->ud;
    const char *tagname = args->tagname;
    int tagid = args->tagid;
    int type = args->type;
    int index = args->index;
    int mainindex = args->mainindex;
    int length = args->length;
    //printf("tagname:%s, type:%d, index:%d, length:%d, mainindex:%d\n", tagname, type, index, length, mainindex);
    PyObject *data = NULL;
    data = PyDict_GetItemString(self->table, tagname);
    if (data == NULL ) {
        //printf("\tnot exist\n");
        return 0;
    }
    if (index > 0) {
        if (!PyList_Check(data)) {
            if (PyDict_Check(data)) {
                PyObject *key, *value;
                Py_ssize_t pos = 0;
                int count = 0;
                while (PyDict_Next(data, &pos, &key, &value)) {
                    count++;
                    if (index == count) {
                        data = value;
                        break;
                    }
                }
                if (index > count) {
                    return 0; //data is finish
                }
            } else {
                PyErr_SetObject(SprotoError, PyString_FromFormat("Expected List or Dict for tagname:%s", tagname));
                return -1;
            }
        } else {
            int len = PyList_Size(data);
            if (index > len) {
                //printf("data is finish\n");
                return 0;
            }
            data = PyList_GetItem(data, index - 1);
        }
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
                *(uint32_t *)args->value = (uint32_t)i;
                return 4;
            } else {
                *(uint64_t *)args->value = (uint64_t)i;
                return 8;
            }
        }
        case SPROTO_TBOOLEAN: {
            //printf("PyBool Check:%s\n", PyBool_Check(data)?"true":"false");
            if (data == Py_True) {
                *(int*)args->value = 1;
            } else {
                *(int*)args->value = 0;
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
            if (len > length) {
                return -1;
            }
            memcpy(args->value, string_ptr, (size_t)len);
            return len+1;
        }
        case SPROTO_TSTRUCT: {
            struct encode_ud sub;
            sub.table = data;
            return sproto_encode(args->subtype, args->value, length, encode, &sub);
        }
        default:
            return 0;
    }
}

void 
sproto_free(PyObject *ptr) {
    struct sproto *sp = PyCapsule_GetPointer(ptr, NULL);
    if (sp != NULL) {
        //printf("release sp\n");
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
    int mainindex;
    PyObject *table;
    PyObject *map_key;
};

static int
decode(const struct sproto_arg *args) {
	struct decode_ud * self = args->ud;
    const char *tagname = args->tagname;
    int tagid = args->tagid;
    int type = args->type;
    int index = args->index;
    int mainindex = args->mainindex;
    int length = args->length;
    //printf("tagname:%s, type:%d, index:%d, length:%d\n", tagname, type, index, length);
    //printf("table pointer: %p\n", (void*)self->table);
    PyObject *obj = self->table;
    PyObject *data = NULL;
    if (index > 0) {
        obj = PyDict_GetItemString(self->table, tagname);
        if (obj == NULL) {
            if (mainindex > 0) {
                PyObject *dict = PyDict_New();
                obj = dict;
            } else {
                PyObject *list = PyList_New(0);
                obj = list;
            }
            PyDict_SetItemString(self->table, tagname, obj);
            Py_DECREF(obj);
        }
    }
    switch(type) {
    case SPROTO_TINTEGER: {
        data = Py_BuildValue("i", *(uint64_t*)args->value);
		break;
	}
	case SPROTO_TBOOLEAN: {
        data = *(int*)args->value > 0 ? Py_True : Py_False;
		break;
	}
	case SPROTO_TSTRING: {
        data = Py_BuildValue("s#", (char*)args->value, length);
		break;
	}
	case SPROTO_TSTRUCT: {
		struct decode_ud sub;
		int r;
        sub.table = PyDict_New();
        if (args->mainindex >= 0) {
            //This struct will set into a map
            sub.mainindex = args->mainindex;
            r = sproto_decode(args->subtype, args->value, length, decode, &sub);
            if (r < 0 || r != length) {
                //printf("after decode:%d\n", r);
                return r;
            }
            //printf("%s\n", PyString_AsString(PyObject_Str(sub.table)));
            PyDict_SetItem(obj, sub.map_key, sub.table);
        } else {
            sub.mainindex = -1;
            data = sub.table;
            r = sproto_decode(args->subtype, args->value, length, decode, &sub);
            //printf("int r:%d\n", r);
            if (r < 0 || r != length) {
                //printf("after decode:%d\n", r);
                return r;
            }
            break;
        }
	}
    }
    if (data != NULL) {
        if (PyList_Check(obj)) {
            PyList_Append(obj, data);
        } else {
            PyDict_SetItemString(self->table, tagname, data);
        }
        Py_DECREF(data);
        if (self->mainindex == tagid) {
            self->map_key = data;
            //printf("match mainindex, data:%s\n", PyString_AsString(PyObject_Str(data)));
        } 
    } else {
        if (self->mainindex == tagid) {
            PyErr_SetString(SprotoError, "map key type not support");
        }
        return NULL;
    }
    return 0;
}

void 
sprototype_free(PyObject *ptr) {
    PyObject *sp_capsule = PyCapsule_GetContext(ptr);
    Py_XDECREF(sp_capsule);
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
        PyObject *st_capsule = PyCapsule_New(st, NULL, sprototype_free);
        PyCapsule_SetContext(st_capsule, sp_ptr);
        Py_XINCREF(sp_ptr);
        return st_capsule;
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
    if (!PyArg_ParseTuple(args, "Os#", &st_capsule, &buffer, &sz)) {
        return NULL;
    }
    self.table = PyDict_New();
    //printf("msg len:%d\n", sz);
    sprototype = PyCapsule_GetPointer(st_capsule, NULL);
    int r = sproto_decode(sprototype, buffer, sz, decode, &self);
    if (r < 0) {
        PyObject_Del(self.table);
        PyErr_SetString(SprotoError, "sproto c lib error");
        return NULL;
    }
    //printf("table size:%d\n", PyDict_Size(self.table));
    return self.table;
}

static PyObject*
py_sproto_pack(PyObject *pymodule, PyObject *args) {
    char *srcbuffer;
    int srcsz;
    if (!PyArg_ParseTuple(args, "s#", &srcbuffer, &srcsz)) {
        return NULL;
    }
    int maxsz = (srcsz + 2047) / 2048 * 2 + (srcsz + 8 - 1) / 8 * 8;
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
    PyObject *var = Py_BuildValue("OOO", ret, py_request, py_response);
    Py_DECREF(ret);
    Py_DECREF(py_request);
    Py_DECREF(py_response);
    return var;
}

static PyObject*
py_sproto_dump(PyObject *pymodule, PyObject *args) {
    PyObject *sp_ptr;
    if (!PyArg_ParseTuple(args, "O", &sp_ptr)) {
        return NULL;
    }
    struct sproto *sp = PyCapsule_GetPointer(sp_ptr, NULL);
    sproto_dump(sp);
    return Py_None;
}

static PyObject*
py_sproto_name(PyObject *pymodule, PyObject *args) {
    PyObject *st_capsule;
    PyObject *ret;
    struct sproto_type *sprototype;
    if (!PyArg_ParseTuple(args, "O", &st_capsule)) {
        return NULL;
    }
    sprototype = PyCapsule_GetPointer(st_capsule, NULL);
    char *name = sproto_name(sprototype);
    ret = Py_BuildValue("s", name);
    return ret;
}

static PyMethodDef pysproto_methods[] = {
    {"sproto_create", py_sproto_create, METH_VARARGS},
    {"sproto_type", py_sproto_type, METH_VARARGS},
    {"sproto_encode", py_sproto_encode, METH_VARARGS},
    {"sproto_decode", py_sproto_decode, METH_VARARGS},
    {"sproto_pack", py_sproto_pack, METH_VARARGS},
    {"sproto_unpack", py_sproto_unpack, METH_VARARGS},
    {"sproto_protocol", py_sproto_protocol, METH_VARARGS},
    {"sproto_dump", py_sproto_dump, METH_VARARGS},
    {"sproto_name", py_sproto_name, METH_VARARGS},
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

