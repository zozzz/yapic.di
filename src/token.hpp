#ifndef TCB2EFA8_B427_441C_80CE_7CB2AA114B2A
#define TCB2EFA8_B427_441C_80CE_7CB2AA114B2A

#include "./di.hpp"

namespace YapicDI {

Token* Token::New(PyObject* name) {
	PyPtr<Token> self = Token::Alloc();
	if (self.IsNull()) {
		return NULL;
	}

    Py_INCREF(name);
    self->name = name;
    self->hash = _Py_HashPointer(self.AsObject());
    if (self->hash == -1) {
        return NULL;
    }

	return self.Steal();
}


PyObject* Token::__new__(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
	static char *kwlist[] = {"name", NULL};

	PyObject* name;
	if (PyArg_ParseTupleAndKeywords(args, kwargs, "U", kwlist, &name)) {
        return (PyObject*) Token::New(name);
	}

	return NULL;
}


Py_hash_t Token::__hash__(Token* self) {
    return self->hash;
}


PyObject* Token::__cmp__(Token* self, PyObject* other, int op) {
    if (op == Py_EQ && Token::CheckExact(other) && self->hash == reinterpret_cast<Token*>(other)->hash) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}


void Token::__dealloc__(Token* self) {
	Py_CLEAR(self->name);
	Super::__dealloc__(self);
}


PyObject* Token::__repr__(Token* self) {
	return PyUnicode_FromFormat("<Token %S>", self->name);
}


} /* end namespace YapicDI */

#endif /* TCB2EFA8_B427_441C_80CE_7CB2AA114B2A */
