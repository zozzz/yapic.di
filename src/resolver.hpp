#ifndef E59C6756_5133_C8FB_12AD_80BD2DE30129
#define E59C6756_5133_C8FB_12AD_80BD2DE30129

#include <yapic/string-builder.hpp>
#include "./di.hpp"

#define ValueResulver_ReturnIfOk(v) \
	if (v != NULL) { \
		return v; \
	} else if (PyErr_Occurred()) { \
		return NULL; \
	}

namespace YapicDI {
	namespace _resolver {
		static inline PyObject* ResolveByKw(Injector* injector, Injector* resolve, PyObject* name, PyObject* type, int recursion) {
			assert(Injector::CheckExact(injector));

			PyObject* kwargs;
			KwOnly* kw;
			PyObject* val;
			do {
				kwargs = injector->kwargs;
				if (kwargs != NULL) {
					assert(PyList_CheckExact(kwargs));

					for (Py_ssize_t i=0 ; i < PyList_GET_SIZE(kwargs); i++) {
						kw = (KwOnly*) PyList_GET_ITEM(kwargs, i);
						assert(KwOnly::CheckExact(kw));
						val = KwOnly::Resolve(kw, resolve, name, type, recursion);
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

		template<bool AllowForwardRef>
		static inline PyObject* GetByType(ValueResolver* self, Injector* injector, Injector* own_injector, PyObject* type, Py_hash_t thash, int recursion) {
			PyObject* result = NULL;

			if (own_injector != NULL) {
				result = Injector::Find(own_injector, type, thash);
			}
			if (result == NULL) {
				result = Injector::Find(injector, type, thash);
			}
			if (result != NULL) {
				assert(Injectable::CheckExact(result));
				return Injectable::Resolve((Injectable*) result, injector, recursion);
			}
			assert(PyErr_Occurred() == NULL);

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


ValueResolver* ValueResolver::New(PyObject* name, PyObject* id, PyObject* default_value, PyObject* injectable) {
	ValueResolver* self = ValueResolver::Alloc();
	if (self == NULL) {
		return NULL;
	}

	Py_XINCREF(name);
	Py_XINCREF(default_value);
	Py_XINCREF(injectable);

	self->name = name;
	self->default_value = default_value;
	self->injectable = injectable;

	ValueResolver::SetId(self, id);
	return self;
}


template<bool UseKwOnly>
PyObject* ValueResolver::Resolve(ValueResolver* self, Injector* injector, Injector* own_injector, int recursion) {
	if (self->id != NULL && Module::State()->Typing.IsForwardDecl(self->id)) {
		PyPtr<> newId = reinterpret_cast<Yapic::ForwardDecl*>(self->id)->Resolve();
		if (newId) {
			ValueResolver::SetId(self, newId);

			if (Module::State()->Typing.IsGenericType(newId)) {
				assert(self->injectable == NULL);
				self->injectable = (PyObject*)Injectable::New(newId, Injectable::Strategy::FACTORY, (PyObject*)NULL);
				if (self->injectable == NULL) {
					return NULL;
				}
			}
		} else {
			PyErr_Clear();
			ValueResolver::SetId(self, reinterpret_cast<Yapic::ForwardDecl*>(self->id)->expr);
		}
	}

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
		result = _resolver::GetByType<true>(self, injector, own_injector, type, self->id_hash, recursion);
		ValueResulver_ReturnIfOk(result);
	}

	if (self->injectable) {
		if (own_injector != NULL) {
			result = Injectable::Resolve(reinterpret_cast<Injectable*>(self->injectable), own_injector, recursion);
			ValueResulver_ReturnIfOk(result);
		}
		result = Injectable::Resolve(reinterpret_cast<Injectable*>(self->injectable), injector, recursion);
		ValueResulver_ReturnIfOk(result);
	}

	if (self->default_value != NULL) {
		Py_INCREF(self->default_value);
		return self->default_value;
	}

	return PyErr_Format(Module::State()->ExcInjectError, YapicDI_Err_InjectableNotFound, self);
}


void ValueResolver::SetId(ValueResolver* self, PyObject* id) {
	Py_XDECREF(self->id);

	if (id != NULL) {
		Py_INCREF(id);
		self->id_hash = PyObject_Hash(id);
		if (self->id_hash == -1) {
			PyErr_Clear();
		}
	} else {
		self->id_hash = -1;
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


int ValueResolver::__traverse__(ValueResolver* self, visitproc visit, void *arg) {
	Py_VISIT(self->name);
	Py_VISIT(self->id);
	Py_VISIT(self->default_value);
	Py_VISIT(self->injectable);
	return 0;
}


int ValueResolver::__clear__(ValueResolver* self) {
	PyObject_GC_UnTrack(self);
	Py_CLEAR(self->name);
	Py_CLEAR(self->id);
	Py_CLEAR(self->default_value);
	Py_CLEAR(self->injectable);
	return 0;
}


void ValueResolver::__dealloc__(ValueResolver* self) {
	Py_CLEAR(self->name);
	Py_CLEAR(self->id);
	Py_CLEAR(self->default_value);
	Py_CLEAR(self->injectable);
	Py_TYPE(self)->tp_free((PyObject*)self);
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

} // end namespace YapicDI

#undef ValueResulver_ReturnIfOk

#endif /* E59C6756_5133_C8FB_12AD_80BD2DE30129 */
