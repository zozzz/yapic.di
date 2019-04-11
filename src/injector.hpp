#ifndef C1001E15_5133_C8FB_12A8_B6171D1397F3
#define C1001E15_5133_C8FB_12A8_B6171D1397F3

#include "./di.hpp"

namespace YapicDI {

Injector* Injector::New(Injector* parent) {
	PyPtr<Injector> self = Injector::Alloc();
	if (self.IsNull()) {
		return NULL;
	}

	if ((self->injectables = PyDict_New()) == NULL) {
		return NULL;
	}

	if ((self->singletons = PyDict_New()) == NULL) {
		return NULL;
	}

	if (parent != NULL) {
		assert(Injector::CheckExact(parent));
		Py_INCREF(parent);
		self->parent = parent;
	}

	return self.Steal();
}


Injector* Injector::Clone(Injector* self) {
	assert(Injector::CheckExact(self));
	return Injector::Clone(self, self->parent);
}


Injector* Injector::Clone(Injector* self, Injector* parent) {
	assert(Injector::CheckExact(self));

	Injector* result = Injector::Alloc();
	if (result == NULL) {
		return NULL;
	}
	result->injectables = self->injectables;
	result->singletons = self->singletons;
	result->kwargs = self->kwargs;
	result->parent = parent;
	Py_XINCREF(result->injectables);
	Py_XINCREF(result->singletons);
	Py_XINCREF(result->kwargs);
	Py_XINCREF(parent);

	return result;
}


PyObject* Injector::Find(Injector* injector, PyObject* id, Py_hash_t hash) {
	assert(Injector::CheckExact((PyObject*) injector));

	PyObject* result;
	do {
		result = _PyDict_GetItem_KnownHash(injector->injectables, id, hash);
	} while (result == NULL && (injector = injector->parent));
	return result;
}


PyObject* Injector::Find(Injector* injector, PyObject* id) {
	assert(Injector::CheckExact((PyObject*) injector));

	Py_hash_t hash;
	if (!PyUnicode_CheckExact(id) || (hash = ((PyASCIIObject*) id)->hash) == -1) {
        if ((hash = PyObject_Hash(id)) == -1) {
            PyErr_Clear();
            return NULL;
        }
    }

	return Injector::Find(injector, id, hash);
}


PyObject* Injector::Provide(Injector* self, PyObject* id) {
	return Injector::Provide(self, id, id, Module::State()->FACTORY, NULL);
}


static inline bool Injector_Set(Injector* self, PyObject* id, PyObject* injectable) {
	Py_hash_t hash = PyObject_Hash(id);
	if (hash == -1) {
		return false;
	}
	reinterpret_cast<Injectable*>(injectable)->hash = hash;
	return _PyDict_SetItem_KnownHash(self->injectables, id, injectable, hash) == 0;
}


PyObject* Injector::Provide(Injector* self, PyObject* id, PyObject* value, PyObject* strategy, PyObject* provide) {
	if (KwOnly::CheckExact(id)) {
		if (!self->kwargs) {
			self->kwargs = PyList_New(1);
			if (self->kwargs == NULL) {
				return NULL;
			}
			Py_INCREF(id);
			PyList_SET_ITEM(self->kwargs, 0, id);
		} else if (PyList_Append(self->kwargs, id) == -1) {
			return NULL;
		}
		Py_RETURN_NONE;
	}

	if (value == NULL) {
		value = id;
	}

	value = reinterpret_cast<PyObject*>(Injectable::New(value, strategy, provide));
	if (value == NULL) {
		return NULL;
	}

	if (Injector_Set(self, id, value)) {
		return value;
	} else {
		Py_DECREF(value);
		return NULL;
	}
}

void Injector::SetParent(Injector* self, Injector* parent) {
	if (self->parent != NULL) {
		Py_DECREF(self->parent);
	}
	if (parent != NULL) {
		Py_INCREF(parent);
	}
	self->parent = parent;
}


PyObject* Injector::__new__(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
	static char *kwlist[] = {"parent", NULL};

	Injector* parent = NULL;
	if (PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &parent)) {
		if (parent != NULL && !Injector::CheckExact(parent)) {
			PyErr_SetString(PyExc_TypeError, "Argument must be 'Injector' instance.");
			return NULL;
		}
		return (PyObject*) Injector::New(parent);
	}

	return NULL;
}


int Injector::__traverse__(Injector* self, visitproc visit, void *arg) {
	Py_VISIT(self->injectables);
	Py_VISIT(self->singletons);
	Py_VISIT(self->kwargs);
	Py_VISIT(self->parent);
	return 0;
}


int Injector::__clear__(Injector* self) {
	PyObject_GC_UnTrack(self);
	Py_CLEAR(self->injectables);
	Py_CLEAR(self->singletons);
	Py_CLEAR(self->kwargs);
	Py_CLEAR(self->parent);
	return 0;
}


PyObject* Injector::provide(Injector* self, PyObject* args, PyObject* kwargs) {
	static char *kwlist[] = {"", "value", "strategy", "provide", NULL};

	PyObject *id=NULL, *value=NULL, *strategy=NULL, *provide=NULL;
	if (PyArg_ParseTupleAndKeywords(args, kwargs, "O|OOO", kwlist, &id, &value, &strategy, &provide)) {
		return Injector::Provide(self, id, value, strategy, provide);
	}
	return NULL;
}

PyObject* Injector::get(Injector* self, PyObject* id) {
	return Injector::__mp_getitem__(self, id);
}

PyObject* Injector::__mp_getitem__(Injector* self, PyObject* id) {
	PyObject* injectable = Injector::Find(self, id); // borrowed
	if (injectable != NULL) {
		assert(Injectable::CheckExact(injectable));
		return Injectable::Resolve((Injectable*) injectable, self, 0);
	} else {
		PyErr_Format(Module::State()->ExcInjectError, YapicDI_Err_InjectableNotFound, id);
		return NULL;
	}
}


int Injector::__mp_setitem__(Injector* self, PyObject* id, PyObject* value) {
	// delete
	if (value == NULL) {
		return PyDict_DelItem(self->injectables, id);
	} else {
		PyPtr<> injectable = reinterpret_cast<PyObject*>(Injectable::New(value, Injectable::Strategy::VALUE, NULL));
		if (injectable && Injector_Set(self, id, injectable)) {
			return 0;
		} else {
			return -1;
		}
	}
}


// TODO: doksiba leírni, hogy ez nem cachel, és ha sűrűn kell meghívni,
// akkor inkább az Injector.provide(...) és az Injector.get(id) kombót javasolt használni
PyObject* Injector::exec(Injector* self, PyObject* args, PyObject* kwargs) {
	static char *kwlist[] = {"callable", "provide", NULL};

	PyObject* callable;
	PyObject* provide = NULL;

	if (PyArg_ParseTupleAndKeywords(args, kwargs, "O|O:exec", kwlist, &callable, &provide)) {
		Injectable* injectable = Injectable::New(callable, Injectable::Strategy::FACTORY, provide);
		if (injectable == NULL) {
			return NULL;
		}
		return Injectable::Resolve(injectable, self, 0);
	}

	return NULL;
}

// vissztér egy olyan azonosítóval, ami használható azonsítóként
// hasznos akkor ha egy sima függvényt akarunk injctálni saját injectableekkel
PyObject* Injector::injectable(Injector* self, PyObject* args, PyObject* kwargs) {
	static char *kwlist[] = {"value", "strategy", "provide", NULL};

	PyObject* value = NULL;
	PyObject* strategy = NULL;
	PyObject* provide = NULL;

	if (PyArg_ParseTupleAndKeywords(args, kwargs, "O|OO:injectable", kwlist, &value, &strategy, &provide)) {
		return (PyObject*)Injectable::New(value, strategy, provide);
	} else {
		return NULL;
	}
}

PyObject* Injector::descend(Injector* self) {
	return (PyObject*) Injector::New(self);
}

} // end namespace YapicDI

#endif /* C1001E15_5133_C8FB_12A8_B6171D1397F3 */
