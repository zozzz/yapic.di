#ifndef TCB2EFA8_B427_441C_80CE_7CB2AA114B2A
#define TCB2EFA8_B427_441C_80CE_7CB2AA114B2A

#include <frameobject.h>
#include "./di.hpp"

namespace YapicDI { namespace Token {
    static PyObject* __repr__(PyTypeObject* self) {
        printf("__repr__\n");
        Yapic::UnicodeBuilder<256> ub;

        ub.AppendAscii('<');
        ub.AppendString("TOKEN ");
        // if (!ub.AppendStringSafe(self->tp_name)) {
        //     return NULL;
        // }

        if (!ub.AppendCharSafe('>')) {
            return NULL;
        }
        return ub.ToPython();
    }

	PyTypeObject* New(PyObject* id) {
        if (!PyUnicode_CheckExact(id)) {
            PyErr_SetString(PyExc_TypeError, "argument 1 must be str");
            return NULL;
        }

        // PyPtr<PyBytesObject> ascii = PyUnicode_AsASCIIString(id);
        // if (!ascii) {
        //     return NULL;
        // }

        PyFrameObject* frame = PyEval_GetFrame();
        if (!frame) {
            return NULL;
        }

        PyPtr<> module = PyObject_GetItem(frame->f_globals, Module::State()->str_name);
        if (!module) {
            return NULL;
        }

        Yapic::Utf8BytesBuilder name;

        if (!name.AppendStringSafe(module)) {
            return NULL;
        }

        if (!name.AppendCharSafe('.')) {
            return NULL;
        }

        if (!name.AppendStringSafe(id)) {
            return NULL;
        }

        PyPtr<PyBytesObject> tokenName = name.ToPython();


        PyType_Slot slots[] = {
            // { Py_tp_repr, __repr__ },
            { 0 },
        };

        PyType_Spec spec = {
            PyBytes_AS_STRING(tokenName),
            sizeof(PyType_Type),
            0,
            Py_TPFLAGS_DEFAULT,
            slots
        };

        return (PyTypeObject*) PyType_FromSpec(&spec);
    }
};  /* end namespace Token */


// Token* Token::New(PyObject* name) {
// 	PyPtr<Token> self = Token::Alloc();
// 	if (self.IsNull()) {
// 		return NULL;
// 	}

//     Py_INCREF(name);
//     self->name = name;
//     self->hash = _Py_HashPointer(self.AsObject());
//     if (self->hash == -1) {
//         return NULL;
//     }

// 	return self.Steal();
// }


// PyObject* Token::__new__(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
// 	static char *kwlist[] = {"name", NULL};

// 	PyObject* name;
// 	if (PyArg_ParseTupleAndKeywords(args, kwargs, "U", kwlist, &name)) {
//         return (PyObject*) Token::New(name);
// 	}

// 	return NULL;
// }


// Py_hash_t Token::__hash__(Token* self) {
//     return self->hash;
// }


// PyObject* Token::__cmp__(Token* self, PyObject* other, int op) {
//     if (op == Py_EQ && Token::CheckExact(other) && self->hash == reinterpret_cast<Token*>(other)->hash) {
//         Py_RETURN_TRUE;
//     } else {
//         Py_RETURN_FALSE;
//     }
// }


// void Token::__dealloc__(Token* self) {
// 	Py_CLEAR(self->name);
// 	Super::__dealloc__(self);
// }


// PyObject* Token::__repr__(Token* self) {
// 	return PyUnicode_FromFormat("<Token %S>", self->name);
// }


} /* end namespace YapicDI */

#endif /* TCB2EFA8_B427_441C_80CE_7CB2AA114B2A */
