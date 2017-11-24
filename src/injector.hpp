#ifndef C1001E15_5133_C8FB_12A8_B6171D1397F3
#define C1001E15_5133_C8FB_12A8_B6171D1397F3

#include "./di.hpp"

namespace ZenoDI {

Injector* Injector::New(Injector* parent, PyObject* scope) {
	Injector* self = Injector::Alloc();
	if (self == NULL) {
		return NULL;
	}

	assert(Injector::Check((PyObject*) parent));
	assert(PyDict_Check(scope));

	Py_INCREF(parent);
	Py_INCREF(scope);

	self->parent = parent;
	self->scope = scope;

	return self;
}


PyObject* Injector::Find(Injector* injector, PyObject* id) {
	assert(Injector::CheckExact((PyObject*) injector));

	do {
		PyObject* result = PyDict_GetItem(injector->scope, id);
		if (result == NULL) {
			injector = injector->parent;
			if (injector == NULL) {
				// TODO: custom error, move up PyErr_Clear
				return NULL;
			} else {
				PyErr_Clear(); // catch error
			}
		} else {
			Py_INCREF(result);
			return result;
		}
	} while (true);

	return NULL;
}


PyObject* Injector::__new__(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
	Injector* self = (Injector*) type->tp_alloc(type, 0);
	if (self == NULL) {
		return NULL;
	}

	self->parent = NULL;

	self->scope = PyDict_New();
	if (self->scope == NULL) {
		Py_DECREF(self);
		return NULL;
	}

	return (PyObject*) self;
}


void Injector::__dealloc__(Injector* self) {
	Py_XDECREF(self->scope);
	Py_XDECREF(self->parent);
	Super::__dealloc__(self);
}

PyObject* Injector::provide(Injector* self, PyObject* args, PyObject* kwargs) {
	static char *kwlist[] = {"", "value", "strategy", "provide", NULL};

	PyObject *id=NULL, *value=NULL, *strategy=NULL, *provide=NULL;
	if (PyArg_ParseTupleAndKeywords(args, kwargs, "O|OOO", kwlist, &id, &value, &strategy, &provide)) {
		if (value == NULL) {
			if (strategy == NULL) {
				strategy = value;
			}
			value = id;
		}

		value = (PyObject*) Provider::New(value, strategy, provide);
		if (value == NULL) {
			return NULL;
		}

		if (PyDict_SetItem(self->scope, id, value) == -1) {
			Py_DECREF(value);
			return NULL;
		}

		return value;
	}
	return NULL;
}

PyObject* Injector::get(Injector* self, PyObject* id) {
	PyObject* provider = PyDict_GetItem(self->scope, id);
	if (provider == NULL) {
		return NULL;
	}
	Py_INCREF(provider);
	return provider;
}

PyObject* Injector::exec(Injector* self, PyObject* args, PyObject* kwargs) {
	static char *kwlist[] = {"callable", "provide", NULL};
	return NULL;
}

PyObject* Injector::injectable(Injector* self, PyObject* args, PyObject* kwargs) {
	static char *kwlist[] = {"value", "strategy", "provide", NULL};
	return NULL;
}

PyObject* Injector::descend(Injector* self) {
	return NULL;
}

} // end namespace ZenoDI

#endif /* C1001E15_5133_C8FB_12A8_B6171D1397F3 */
