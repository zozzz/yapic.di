#ifndef C1001E15_5133_C8FB_12A8_B6171D1397F3
#define C1001E15_5133_C8FB_12A8_B6171D1397F3

#include "./module.hpp"

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


static PyObject* Injector_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
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


static void Injector_dealloc(Injector* self) {
	Py_XDECREF(self->scope);
	Py_XDECREF(self->parent);
	Py_TYPE(self)->tp_free((PyObject*) self);
}

PyDoc_STRVAR(Injector_provide_doc, "");
static PyObject* Injector_provide(Injector* self, PyObject* args, PyObject* kwargs) {
	static char *kwlist[] = {"", "value", "strategy", "provide", NULL};

	PyObject *id=NULL, *value=NULL, *strategy=NULL, *provide=NULL;
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|OOO", kwlist, &id, &value, &strategy, &provide)) {
		return NULL;
	}

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

PyDoc_STRVAR(Injector_get_doc, "");
static PyObject* Injector_get(Injector* self, PyObject* id) {
	PyObject* provider = PyDict_GetItem(self->scope, id);
	if (provider == NULL) {
		return NULL;
	}
	Py_INCREF(provider);
	return provider;
}

PyDoc_STRVAR(Injector_exec_doc, "");
static PyObject* Injector_exec(Injector* self, PyObject* args, PyObject* kwargs) {
	static char *kwlist[] = {"callable", "provide", NULL};
	return NULL;
}

PyDoc_STRVAR(Injector_injectable_doc, "");
static PyObject* Injector_injectable(Injector* self, PyObject* args, PyObject* kwargs) {
	static char *kwlist[] = {"value", "strategy", "provide", NULL};
	return NULL;
}

PyDoc_STRVAR(Injector_descend_doc, "");
static PyObject* Injector_descend(Injector* self) {
	return NULL;
}

static PyMethodDef Injector_methods[] = {
	{"provide", (PyCFunction) Injector_provide, METH_VARARGS | METH_KEYWORDS, Injector_provide_doc},
	{"get", (PyCFunction) Injector_get, METH_O, Injector_get_doc},
	{"exec", (PyCFunction) Injector_exec, METH_VARARGS | METH_KEYWORDS, Injector_exec_doc},
	{"injectable", (PyCFunction) Injector_injectable, METH_VARARGS | METH_KEYWORDS, Injector_injectable_doc},
	{"descend", (PyCFunction) Injector_descend, METH_NOARGS, Injector_descend_doc},
	NULL
};


PyTypeObject ZenoDI_TypeVar(Injector) = {
	PyVarObject_HEAD_INIT(NULL, 0)
	/* tp_name */ 			"zeno.di.Injector",
	/* tp_basicsize */ 		sizeof(Injector),
	/* tp_itemsize */ 		0,
	/* tp_dealloc */ 		(destructor) Injector_dealloc,
	/* tp_print */ 			0,
	/* tp_getattr */ 		0,
	/* tp_setattr */ 		0,
	/* tp_as_async */ 		0,
	/* tp_repr */ 			0,
	/* tp_as_number */ 		0,
	/* tp_as_sequence */ 	0,
	/* tp_as_mapping */ 	0,
	/* tp_hash  */ 			0,
	/* tp_call */ 			0,
	/* tp_str */ 			0,
	/* tp_getattro */ 		0,
	/* tp_setattro */ 		0,
	/* tp_as_buffer */ 		0,
	/* tp_flags */ 			Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	/* tp_doc */ 			0,
	/* tp_traverse */ 		0,
	/* tp_clear */ 			0,
	/* tp_richcompare */ 	0,
	/* tp_weaklistoffset */ 0,
	/* tp_iter */ 			0,
	/* tp_iternext */ 		0,
	/* tp_methods */ 		Injector_methods,
	/* tp_members */ 		0,
	/* tp_getset */ 		0,
	/* tp_base */ 			0,
	/* tp_dict */ 			0,
	/* tp_descr_get */ 		0,
	/* tp_descr_set */ 		0,
	/* tp_dictoffset */ 	0,
	/* tp_init */ 			0,
	/* tp_alloc */ 			0,
	/* tp_new */ 			Injector_new,
	/* tp_free */ 			0
};

} // end namespace ZenoDI

#endif /* C1001E15_5133_C8FB_12A8_B6171D1397F3 */
