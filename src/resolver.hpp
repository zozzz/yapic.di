#ifndef E59C6756_5133_C8FB_12AD_80BD2DE30129
#define E59C6756_5133_C8FB_12AD_80BD2DE30129

#include <yapic/string-builder.hpp>
#include "./di.hpp"
#include "./util.hpp"

#define ValueResulver_ReturnIfOk(v) \
	if (v != NULL) { \
		return v; \
	} else if (PyErr_Occurred()) { \
		return NULL;\
	} \

namespace ZenoDI {
	namespace _resolver {
		static inline PyObject* ResolveByType(Injector* injector, Injector* resolve, PyObject* type, int recursion) {
			assert(Injector::CheckExact(injector));

			do {
				PyObject* result = PyDict_GetItem(injector->scope, type);
				if (result != NULL) {
					assert(Injectable::CheckExact(result));
					return Injectable::Resolve((Injectable*) result, resolve, recursion);
				}
			} while (injector = injector->parent);
			return NULL;
		}

		static inline PyObject* ResolveByKw(Injector* injector, Injector* resolve, PyObject* name, PyObject* type, int recursion) {
			assert(Injector::CheckExact(injector));

			do {
				PyObject* kwargs = injector->kwargs;
				if (kwargs != NULL) {
					assert(PyList_CheckExact(kwargs));

					for (Py_ssize_t i=0 ; i < PyList_GET_SIZE(kwargs); i++) {
						KwOnly* kw = (KwOnly*) PyList_GET_ITEM(kwargs, i);
						assert(KwOnly::CheckExact(kw));
						PyObject* val = KwOnly::Resolve(kw, resolve, name, type, recursion);
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

		template<bool AllowForwardRef>
		static inline PyObject* GetByType(ValueResolver* self, Injector* injector, Injector* own_injector, PyObject* type, int recursion) {
			PyObject* result;

			if (own_injector != NULL) {
				result = _resolver::ResolveByType(own_injector, injector, type, recursion);
				ValueResulver_ReturnIfOk(result)
			}
			result = _resolver::ResolveByType(injector, injector, type, recursion);
			ValueResulver_ReturnIfOk(result)

			// maybe forwardref...
			if (AllowForwardRef) {
				PyObject* globals = self->globals;
				if (PyUnicode_CheckExact(type) && globals != NULL) {
					PyObject* fwType = ResolveNameFromGlobals(globals, type);
					if (fwType != NULL) {
						result = GetByType<false>(self, injector, own_injector, fwType, recursion);
						Py_DECREF(fwType);
						ValueResulver_ReturnIfOk(result)
					} else if (PyErr_Occurred()) {
						PyErr_Clear();
					}
				}
			}

			// get Injector
			if ((PyTypeObject*) type == const_cast<PyTypeObject*>(Injector::PyType())) {
				if (own_injector != NULL) {
					return (PyObject*) Injector::Clone(own_injector, injector);
				} else {
					Py_INCREF(injector);
					return (PyObject*) injector;
				}
			}

			return NULL;
		}
	} /* end namespace _resolver */


ValueResolver* ValueResolver::New(PyObject* name, PyObject* id, PyObject* default_value, PyObject* globals) {
	ValueResolver* self = ValueResolver::Alloc();
	if (self == NULL) {
		return NULL;
	}

	Py_XINCREF(name);
	Py_XINCREF(id);
	Py_XINCREF(default_value);
	Py_XINCREF(globals);

	self->name = name;
	self->id = id;
	self->default_value = default_value;
	self->globals = globals;

	return self;
}


template<bool UseKwOnly>
PyObject* ValueResolver::Resolve(ValueResolver* self, Injector* injector, Injector* own_injector, int recursion) {
	// TODO: unpack type, handle Optional[...]
	PyObject* type = self->id;
	PyObject* result;

	if (UseKwOnly) {
		if (self->name) {
			if (own_injector != NULL) {
				result = _resolver::ResolveByKw(own_injector, injector, self->name, type, recursion);
				ValueResulver_ReturnIfOk(result)
			}
			result = _resolver::ResolveByKw(injector, injector, self->name, type, recursion);
			ValueResulver_ReturnIfOk(result)
		}
	}

	if (type) {
		result = _resolver::GetByType<true>(self, injector, own_injector, type, recursion);
		ValueResulver_ReturnIfOk(result);
	}

	if (self->default_value != NULL) {
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
	Py_CLEAR(self->globals);
	Super::__dealloc__(self);
}


PyObject* ValueResolver::__repr__(ValueResolver* self) {
	Yapic::UnicodeBuilder<256> builder;
	if (!builder.AppendStringSafe("<ValueResolver")) {
		return NULL;
	}

	if (self->name) {
		if (!builder.AppendStringSafe(" name=")) {
			return NULL;
		}
		if (!builder.AppendStringSafe(self->name)) {
			return NULL;
		}
	}

	if (self->id) {
		if (!builder.AppendStringSafe(" id=")) {
			return NULL;
		}
		PyPtr<> repr = PyObject_Repr(self->id);
		if (repr.IsNull()) {
			return NULL;
		}
		if (!builder.AppendStringSafe(repr)) {
			return NULL;
		}
	}

	if (self->default_value) {
		if (!builder.AppendStringSafe(" default=")) {
			return NULL;
		}
		PyPtr<> repr = PyObject_Repr(self->default_value);
		if (repr.IsNull()) {
			return NULL;
		}
		if (!builder.AppendStringSafe(repr)) {
			return NULL;
		}
	}

	if (!builder.AppendStringSafe(">")) {
		return NULL;
	}

	return builder.ToPython();
}

} // end namespace ZenoDI

#undef ValueResulver_ReturnIfOk

#endif /* E59C6756_5133_C8FB_12AD_80BD2DE30129 */
