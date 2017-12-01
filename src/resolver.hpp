#ifndef E59C6756_5133_C8FB_12AD_80BD2DE30129
#define E59C6756_5133_C8FB_12AD_80BD2DE30129

#include "./di.hpp"

namespace ZenoDI {
	namespace _resolver {
		static inline PyObject* ResolveByType(Injector* injector, Injector* resolve, PyObject* type) {
			assert(Injector::CheckExact(injector));

			do {
				PyObject* result = PyDict_GetItem(injector->scope, type);
				if (result != NULL && Injectable::CheckExact(result)) {
					return Injectable::Resolve((Injectable*) result, resolve);
				}
			} while (injector = injector->parent);
			return NULL;
		}

		static inline PyObject* ResolveByKw(Injector* injector, Injector* resolve, PyObject* name, PyObject* type) {
			assert(Injector::CheckExact(injector));

			do {
				PyObject* kwargs = injector->kwargs;
				if (kwargs != NULL) {
					assert(PyList_CheckExact(kwargs));

					for (Py_ssize_t i=0 ; i < PyList_GET_SIZE(kwargs); i++) {
						KwOnly* kw = (KwOnly*) PyList_GET_ITEM(kwargs, i);
						assert(KwOnly::CheckExact(kw));
						PyObject* val = KwOnly::Resolve(kw, resolve, name, type);
						if (val == NULL && PyErr_Occurred()) {
							return NULL;
						} else {
							return val;
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


#define ValueResulver_ReturnIfOk(v) \
	if (v != NULL) { \
		return v; \
	} else if (PyErr_Occurred()) { \
		return NULL;\
	} \


template<bool UseKwOnly>
PyObject* ValueResolver::Resolve(ValueResolver* self, Injector* injector, Injector* own_injector) {
	// TODO: unpack type, handle Optional[...]
	PyObject* type = self->id;
	PyObject* result;

	if (UseKwOnly) {
		if (self->name) {
			if (own_injector != NULL) {
				result = _resolver::ResolveByKw(own_injector, injector, self->name, type);
				ValueResulver_ReturnIfOk(result)
			}
			result = _resolver::ResolveByKw(injector, injector, self->name, type);
			ValueResulver_ReturnIfOk(result)
		}
	}

	if (type) {
		if (own_injector != NULL) {
			result = _resolver::ResolveByType(own_injector, injector, type);
			ValueResulver_ReturnIfOk(result)
		}
		result = _resolver::ResolveByType(injector, injector, type);
		ValueResulver_ReturnIfOk(result)

		if ((PyTypeObject*) type == const_cast<PyTypeObject*>(Injector::PyType())) {
			if (own_injector != NULL) {
				return (PyObject*) Injector::Clone(own_injector, injector);
			} else {
				Py_INCREF(injector);
				return (PyObject*) injector;
			}
		}
	}

	if (self->default_value != NULL) {
		Py_INCREF(self->default_value);
		return self->default_value;
	}

	return PyErr_Format(Module::State()->ExcInjectError, ZenoDI_Err_InjectableNotFound, self);
}

#undef ValueResulver_ReturnIfOk

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
