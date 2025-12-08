#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include "sproto.h"
#ifdef _WIN32
#include "msvcint.h"
#endif

typedef int bool;
#define true 1
#define false 0


#define GET_TYPE_NAME(obj) (Py_TYPE(obj)->tp_name)
static PyObject *SprotoError;

struct encode_ud {
    PyObject *table;
    PyObject *values;
};

void
mysetstring(PyObject *tmp, char *str) {
	printf("set string:%s\n", str);
	//PyErr_SetString(SprotoError, str);
	PyErr_SetString(SprotoError, str);
}

void
mysetobject(PyObject *tmp, PyObject *unicode_str) {
	PyObject_Print(unicode_str, stdout, Py_PRINT_RAW);
	PyErr_SetObject(SprotoError, unicode_str);
}

static int
encode(const struct sproto_arg *args) {
    struct encode_ud *self = args->ud;
    const char *tagname = args->tagname;
    int type = args->type;
    int index = args->index;
    int length = args->length;
    //printf("tagname:%s, type:%d, index:%d, length:%d, subtype:%d\n", tagname, type, index, length, args->subtype);
    PyObject *data = NULL;
    data = PyDict_GetItemString(self->table, tagname);
    if (data == NULL ) {
        if (index > 0) {
            //printf("%s\t no array\n", tagname);
            return SPROTO_CB_NOARRAY;
        }
        //printf("%s\tnot exist\n", tagname);
        return SPROTO_CB_NIL;
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
                    //printf("data is finish, index:%d, len:%d\n", index, count);
                    return SPROTO_CB_NIL; //data is finish
                }
            } else {
                // py2 PyString_FromFormat py3 PyUnicode_FromFormat
                mysetobject(SprotoError, PyUnicode_FromFormat("Expected List or Dict for tagname:%s", tagname));
                return SPROTO_CB_ERROR;
            }
        } else {
            int len = PyList_Size(data);
            if (index > len) {
                //printf("data is finish, index:%d, len:%d\n", index, len);
                return SPROTO_CB_NIL;
            }
            data = PyList_GetItem(data, index - 1);
        }
    }
    //self->tagname = tagname;
    switch(type) {
        case SPROTO_TINTEGER: {
            //if (PyInt_Check(data) || PyLong_Check(data) || ((args->extra) && PyFloat_Check(data))) { //py2
            if (PyLong_Check(data) || ((args->extra) && PyFloat_Check(data))) { //py2
                long long i;
                if (args->extra) {
                    i = PyFloat_AsDouble(data) * args->extra + 0.5;
                    //printf("input data:%lld\n", i);
                } else {
                    i = PyLong_AsLongLong(data);
                }
                long long vh = i >> 31; 
                if (vh == 0 || vh == -1) {
                    *(int32_t *)args->value = (int32_t)i;
                    return 4;
                } else {
                    *(int64_t *)args->value = (int64_t)i;
                    return 8; 
                }
            } else {
                // py2 PyString_FromFormat py3 PyUnicode_FromFormat
                mysetobject(SprotoError, PyUnicode_FromFormat("type mismatch, tag:%s, expected int or long, got:%s\n", tagname, GET_TYPE_NAME(data)));
                return -1;
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
            //printf("PyString Check:%s\n", PyBytes_Check(data)?"true":"false");
            Py_ssize_t len = 0;
            const char* string_ptr = NULL;
            PyObject *tmp_str = NULL;
            if (args->extra == SPROTO_TSTRING_BINARY) { //binary string
                //printf("sproto tstring binary:%d, bytes_check:%d\n", PyUnicode_Check(data), PyBytes_Check(data));
                // py2 PyString_Check  py3 PyBytes_Check
                if (!PyBytes_Check(data)) {
                    // py2 PyString_FromFormat py3 PyUnicode_FromFormat
                    mysetobject(SprotoError, PyUnicode_FromFormat("type mismatch, tag:%s, expected bytes, got:%s\n", tagname, GET_TYPE_NAME(data)));
                    return SPROTO_CB_ERROR;
                }
                PyBytes_AsStringAndSize(data, (char**)&string_ptr, &len);
            } else {    //unicode string
                if (!PyUnicode_Check(data)) {
                    // py2 PyString_FromFormat py3 PyUnicode_FromFormat
                    mysetobject(SprotoError, PyUnicode_FromFormat("type mismatch, tag:%s, expected unicode, got:%s\n", tagname, GET_TYPE_NAME(data)));
                    return SPROTO_CB_ERROR;
                }
                // py2 PyString_AsStringAndSize py3 PyBytes_AsStringAndSize
                string_ptr = PyUnicode_AsUTF8AndSize(data, &len);
            }
            if (len > length) {
                return SPROTO_CB_ERROR;
            }
            memcpy(args->value, string_ptr, (size_t)len);
            if (tmp_str != NULL) {
                Py_XDECREF(tmp_str);
            }
            return len;
        }
        case SPROTO_TSTRUCT: {
            struct encode_ud sub;
            sub.table = data;
            //printf("encode SPROTO_TSTRUCT\n");
            int r = sproto_encode(args->subtype, args->value, length, encode, &sub);
            if (r < 0) {
                //printf("sproto cb error\n");
                return SPROTO_CB_ERROR;
            }
            return r;
        }
        case SPROTO_TDOUBLE: {
            if (PyFloat_Check(data)) {
                double d = PyFloat_AsDouble(data);
                *(double *)args->value = d;
                return 8;
            } else {
                mysetobject(SprotoError, PyUnicode_FromFormat("type mismatch, tag:%s, expected float, got:%s\n", tagname, GET_TYPE_NAME(data)));
                return -1;
            }
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
    Py_ssize_t sz = 0;
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
    Py_ssize_t length = args->length;
    //printf("tagname:%s, type:%d, index:%d, length:%d, mainindex:%d\n", tagname, type, index, length, mainindex);
    //printf("table pointer: %p\n", (void*)self->table);
    PyObject *obj = self->table;
    PyObject *data = NULL;
    if (index != 0) {
        obj = PyDict_GetItemString(self->table, tagname);
        if (obj == NULL) {
            if (mainindex >= 0) {
                PyObject *dict = PyDict_New();
                obj = dict;
            } else {
                PyObject *list = PyList_New(0);
                obj = list;
            }
            PyDict_SetItemString(self->table, tagname, obj);
            Py_XDECREF(obj);
            if (index < 0) {
                return 0;
            }
        }
    }
    switch(type) {
    case SPROTO_TINTEGER: {
        long long i = (long long)args->value;
        if (args->extra) {
            int64_t tmp = *(int64_t*)i;
            double result = (double)tmp / args->extra;
            data = Py_BuildValue("d", result);
            break;
        } else if (length == 4) {
            data = Py_BuildValue("i", *(int32_t*)i);
            break;
        } else if (length == 8) {
            data = Py_BuildValue("L", *(int64_t*)i);
            break;
        } else {
            mysetstring(SprotoError, "unexpected integer length");
            return SPROTO_CB_ERROR;
        }
	}
	case SPROTO_TBOOLEAN: {
        data = PyBool_FromLong(*(int*)args->value);
		break;
	}
	case SPROTO_TSTRING: {
        if (args->extra == SPROTO_TSTRING_STRING) {
            data = PyUnicode_DecodeUTF8((char*)args->value, length, "decode utf8-encode unicode string");
        } else {
            data = Py_BuildValue("y#", (char*)args->value, length);
        }
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
            if (r < 0) {
                //printf("after decode:%d\n", r);
                Py_DECREF(sub.table);
                return SPROTO_CB_ERROR;
            }
            if (r != length) {
                Py_DECREF(sub.table);
                return r;
            }
            // printf("%s:%s\n", PyString_AsString(PyObject_Str(sub.map_key)), PyString_AsString(PyObject_Str(sub.table)));
            PyDict_SetItem(obj, sub.map_key, sub.table);
            Py_DECREF(sub.table);
        } else {
            sub.mainindex = -1;
            data = sub.table;
            r = sproto_decode(args->subtype, args->value, length, decode, &sub);
            //printf("int r:%d\n", r);
            if (r < 0) {
                //printf("after decode:%d\n", r);
                Py_DECREF(sub.table);
                return SPROTO_CB_ERROR;
            }
            if (r != length) {
                Py_DECREF(sub.table);
                return r;
            }
        }
        break;
	}
    case SPROTO_TDOUBLE: {
        double d = *(double *)args->value;
        data = Py_BuildValue("d", d);
        break;
    }
    default: {
            mysetobject(SprotoError, PyUnicode_FromFormat("unexpected type: %d", type));
            return SPROTO_CB_ERROR;
        }
    }
    if (data != NULL) {
        if (PyList_Check(obj)) {
            PyList_Append(obj, data);
        } else {
            PyDict_SetItemString(self->table, tagname, data);
        }
        Py_XDECREF(data);
        if (self->mainindex == tagid) {
            self->map_key = data;
            //printf("match mainindex, data:%s\n", PyString_AsString(PyObject_Str(data)));
        } 
    } else {
        if (self->mainindex == tagid) {
            mysetstring(SprotoError, "map key type not support");
            return SPROTO_CB_ERROR;
        }
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
        Py_ssize_t r = sproto_encode(sprototype, buffer, sz, encode, &self);
        if (r < 0) {
            if (PyErr_Occurred()) {
                PyMem_Free(buffer);
                return NULL;
            }
            buffer = PyMem_Realloc(buffer, sz * 2);
            sz = sz * 2;
        } else {
            PyObject *t = Py_BuildValue("y#", buffer, r);
            PyMem_Free(buffer);
            return t;
        }
    }
}

static PyObject*
py_sproto_decode(PyObject *pymodule, PyObject *args) {
    PyObject *st_capsule;
    char *buffer;
    Py_ssize_t sz = 0;
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
        Py_XDECREF(self.table);
        mysetstring(SprotoError, "sproto c lib error");
        return NULL;
    }
    //printf("table size:%d\n", PyDict_Size(self.table));
    PyObject *t = Py_BuildValue("Ni", self.table, r);
    return t;
}

static PyObject*
py_sproto_pack(PyObject *pymodule, PyObject *args) {
    char *srcbuffer;
    Py_ssize_t srcsz;
    if (!PyArg_ParseTuple(args, "y#", &srcbuffer, &srcsz)) {
        printf("py_sproto_pack args check fail\n");
        return NULL;
    }
    int maxsz = (srcsz + 2047) / 2048 * 2 + srcsz + 2;
    void *dstbuffer = PyMem_Malloc(maxsz);
    Py_ssize_t bytes = sproto_pack(srcbuffer, srcsz, dstbuffer, maxsz);
    if (bytes > maxsz) {
        return Py_None;
    }
    PyObject *t = Py_BuildValue("y#", dstbuffer, bytes);
    PyMem_Free(dstbuffer);
    return t; 
}

static PyObject*
py_sproto_unpack(PyObject *pymodule, PyObject *args) {
    char *srcbuffer;
    Py_ssize_t srcsz;
    if (!PyArg_ParseTuple(args, "y#", &srcbuffer, &srcsz)) {
        return NULL;
    }
    Py_ssize_t dstsz = 1024;
    void *dstbuffer = PyMem_Malloc(dstsz);
    int r = sproto_unpack(srcbuffer, srcsz, dstbuffer, dstsz);
    if (r < 0) {
        PyMem_Free(dstbuffer);
        return Py_None;
    }
    if (r > dstsz) {
        void *new_ptr = PyMem_Realloc(dstbuffer, r);
        if (new_ptr == NULL) {
            PyMem_Free(dstbuffer);
            // py2 PyString_FromFormat py3 PyUnicode_FromFormat
            mysetobject(SprotoError, PyUnicode_FromFormat("out of memory"));
            return Py_None;
        }
        dstbuffer = new_ptr;
    }
    dstsz = r;
    r = sproto_unpack(srcbuffer, srcsz, dstbuffer, dstsz);
    if (r < 0) {
        PyMem_Free(dstbuffer);
        return Py_None;
    }
    PyObject *t = Py_BuildValue("y#", dstbuffer, dstsz);
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
    //if (PyInt_Check(name_or_tagid)) { //py2
    if (PyLong_Check(name_or_tagid)) //py3
    {
        // tagid = PyInt_AsLong(name_or_tagid); //py2
        tagid = PyLong_AsLong(name_or_tagid); //py3
        if (PyErr_Occurred()) {
            return NULL;
        }
        name = sproto_protoname(sp, tagid);
        ret = Py_BuildValue("s", name);
    } 
    else 
    {
        name = PyUnicode_AsUTF8(name_or_tagid);
        //name = PyString_AsString(name_or_tagid); //py2
        tagid = sproto_prototag(sp, name);
        ret = Py_BuildValue("i", tagid);
    }
    struct sproto_type *request;
    struct sproto_type *response;
    PyObject *py_request;
    PyObject *py_response;
    request = sproto_protoquery(sp, tagid, SPROTO_REQUEST);
    py_request = request == NULL ? Py_None:PyCapsule_New(request, NULL, NULL);
    response = sproto_protoquery(sp, tagid, SPROTO_RESPONSE);
    py_response = response == NULL? Py_None:PyCapsule_New(response, NULL, NULL);
    PyObject *var = Py_BuildValue("NNN", ret, py_request, py_response);
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
    const char *name = sproto_name(sprototype);
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

static struct PyModuleDef module = {
    PyModuleDef_HEAD_INIT,
    "pysproto",
    "docstring for the module",
    -1,
    pysproto_methods
};

//py2
// PyMODINIT_FUNC
// initpysproto(void){
//py3 
PyMODINIT_FUNC 
PyInit_pysproto() {    
    SprotoError = PyErr_NewException("pysproto.error", NULL, NULL);
    Py_INCREF(SprotoError);
    PyObject *m = PyModule_Create(&module);
    if (m == NULL) {
        printf("module create failed\n");
        return NULL;
    }
    PyModule_AddObject(m, "error", SprotoError);
    return m;
}
