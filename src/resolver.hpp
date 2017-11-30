#ifndef E59C6756_5133_C8FB_12AD_80BD2DE30129
#define E59C6756_5133_C8FB_12AD_80BD2DE30129

#include "./di.hpp"

namespace ZenoDI {
	namespace _resolver {
		static inline PyObject* ResolveByType(Injector* injector, PyObject* type) {
			Injectable* injectable = (Injectable*) Injector::Find(injector, type);
			if (injectable == NULL) {
				return NULL;
			} else if (Injectable::CheckExact(injectable)) {
				return Injectable::Resolve(injectable, injector);
			} else {
				return NULL;
			}
		}

		static inline PyObject* ResolveByKw(Injector* injector, PyObject* name, PyObject* type) {
			assert(Injector::CheckExact(injector));

			do {
				if (injector->kwargs) {
					assert(PyList_CheckExact(injector->kwargs));
					PyObject* kwargs = injector->kwargs;

					for (Py_ssize_t i=0 ; i < PyList_GET_SIZE(kwargs); i++) {
						KwOnly* kw = (KwOnly*) PyList_GET_ITEM(kwargs, i);
						assert(KwOnly::CheckExact(kw));
						PyObject* val = KwOnly::Resolve(kw, injector, name, type);
						if (val != NULL) {
							return val;
						} else if (PyErr_Occurred()) {
							return NULL;
						}
					}
				}
			} while (injector = injector->parent);

			return NULL;
		}
	} /* end namespace _resolver */


ValueResolver* ValueResolver::New(PyObject* name, PyObject* id, PyObject* default_value) {
	ValueResolver* self = ValueResolver::Alloc();
	if (self == NULL) {
		return NULL;
	}

	if (name != NULL) { Py_INCREF(name); }
	if (id != NULL) { Py_INCREF(id); }
	if (default_value != NULL) { Py_INCREF(default_value); }

	self->name = name;
	self->id = id;
	self->default_value = default_value;

	return self;
}


template<bool UseKwOnly>
PyObject* ValueResolver::Resolve(ValueResolver* self, Injector* injector) {
	// TODO: unpack type, handle Optional[...]
	PyObject* type = self->id;

	if (UseKwOnly) {
		if (self->name) {
			PyObject* byKw = _resolver::ResolveByKw(injector, self->name, type);
			if (byKw != NULL) {
				return byKw;
			} else if (PyErr_Occurred()) {
				return NULL;
			}
		}
	}

	if (type) {
		PyObject* byType = _resolver::ResolveByType(injector, type);
		if (byType != NULL) {
			return byType;
		} else if (PyErr_Occurred()) {
			return NULL;
		}
	}

	if (self->default_value) {
		Py_INCREF(self->default_value);
		return self->default_value;
	}

	return PyErr_Format(Module::State()->ExcInjectError, ZenoDI_Err_InjectableNotFound, self);
}


void ValueResolver::SetId(ValueResolver* self, PyObject* id) {
	if (self->id != NULL) {
		Py_DECREF(self->id);
	}
	if (id != NULL) {
		Py_INCREF(id);
	}
	self->id = id;
}


void ValueResolver::SetName(ValueResolver* self, PyObject* name) {
	if (self->name != NULL) {
		Py_DECREF(self->name);
	}
	if (name != NULL) {
		Py_INCREF(name);
	}
	self->name = name;
}


void ValueResolver::SetDefaultValue(ValueResolver* self, PyObject* value) {
	if (self->default_value != NULL) {
		Py_DECREF(self->default_value);
	}
	if (value != NULL) {
		Py_INCREF(value);
	}
	self->default_value = value;
}


void ValueResolver::__dealloc__(ValueResolver* self) {
	Py_CLEAR(self->name);
	Py_CLEAR(self->id);
	Py_CLEAR(self->default_value);
	Super::__dealloc__(self);
}


PyObject* ValueResolver::__repr__(ValueResolver* self) {
	PyObject* idname;
	if (self->id == NULL) {
		idname = Py_None;
		Py_INCREF(idname);
	} else {
		idname = PyObject_GetAttr(self->id, Module::State()->STR_QUALNAME);
		if (idname == NULL) {
			PyErr_Clear();
			Py_INCREF(self->id);
			idname = self->id;
		}
	}

	PyObject* repr;
	if (self->default_value) {
		repr = PyUnicode_FromFormat("<ValueResolver name=%R, id=%S, default=%R>",
			self->name, idname, self->default_value);
	} else {
		repr = PyUnicode_FromFormat("<ValueResolver name=%R, id=%S>",
			self->name, idname);
	}
	Py_DECREF(idname);
	return repr;
}

} // end namespace ZenoDI

#endif /* E59C6756_5133_C8FB_12AD_80BD2DE30129 */
