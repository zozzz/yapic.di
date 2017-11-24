#ifndef AC1E491D_0133_C8FB_12AA_F65E6B95E8A0
#define AC1E491D_0133_C8FB_12AA_F65E6B95E8A0

#include "./di.hpp"

namespace ZenoDI {

namespace _provider {
	static inline PyObject* NewValueResolver(PyObject* name, PyObject* annots, PyObject* defaultValue) {
		assert(name != NULL && PyUnicode_Check(name));
		assert(annots == NULL || PyDict_Check(annots));

		PyObject* id = annots == NULL ? NULL : PyDict_GetItem(annots, name); // borrowed ref, no need decref
		if (id == NULL && PyErr_Occurred()) {
			PyErr_Clear();
		}

		return (PyObject*) ValueResolver::New(name, id, defaultValue);
	}

	namespace Collect {
		/**
		 * def fn_name(arg1, arg2)
		 * 		+ normal function
		 * 		+ has __code__ object
		 *
		 * class X:
		 *     def method(self, a1):
		 * 			+ bound method (has __self__)
		 * 			+ has __code__
		 */
		namespace Callable {
			static inline PyObject* GetCallable(PyObject* callable, PyPtr<PyCodeObject>& code) {
				code = PyObject_GetAttr(callable, Module::State()->STR_CODE);
				if (code.IsNull()) {
					PyErr_Clear();
					/**
					 * class X:
					 *     def __call__(self): pass
					 */
					PyPtr<> __call__ = PyObject_GetAttr(callable, Module::State()->STR_CALL);
					if (__call__.IsValid()) {
						if (PyObject_IsInstance(__call__, Module::State()->MethodWrapperType)) {
							// got builtin or any other unsupported callable object
							PyErr_Format(Module::State()->ExcProvideError, ZenoDI_Err_GotBuiltinCallable, callable);
							return NULL;
						} else {
							return GetCallable(__call__, code);
						}
					}
				} else if (PyCallable_Check(callable)) {
					/**
					 * Normal functions:
					 * - def fn(): pass
					 * - x().bounded
					 */
					Py_INCREF(callable);
					return callable;
				}
				// got builtin or any other unsupported callable object
				PyErr_Format(Module::State()->ExcProvideError, ZenoDI_Err_GotBuiltinCallable, callable);
				return NULL;
			}

			static inline bool Arguments(Provider* provider, PyCodeObject* code,
										 PyObject* defaults, PyObject* kwdefaults, PyObject* annots, int offset) {

				int argcount = code->co_argcount - offset;
				assert(argcount >= 0);

				if (argcount) {
					assert(PyTuple_CheckExact(code->co_varnames));

					PyPtr<> args = PyTuple_New(argcount);
					if (args.IsNull()) {
						return false;
					}

					int defcount = (defaults == NULL ? 0 : (int) PyTuple_GET_SIZE(defaults));
					int defcounter = 0;
					for (int i=offset ; i<code->co_argcount ; ++i) {
						assert(PyTuple_GET_SIZE(code->co_varnames) > i);

						PyObject* def = (defcount && code->co_argcount - defcount <= i
							? PyTuple_GET_ITEM(defaults, defcounter++)
							: NULL);
						PyObject* resolver = NewValueResolver(PyTuple_GET_ITEM(code->co_varnames, i), annots, def);
						if (resolver == NULL) {
							return false;
						}
						PyTuple_SET_ITEM(args, i - offset, resolver);
					}

					provider->args = args.Steal();
				}

				if (code->co_kwonlyargcount) {
					PyPtr<> kwargs = PyDict_New();
					if (kwargs.IsNull()) {
						return false;
					}

					for (int i=code->co_argcount ; i<code->co_kwonlyargcount + code->co_argcount ; i++) {
						assert(PyTuple_GET_SIZE(code->co_varnames) > i);
						PyObject* name = PyTuple_GET_ITEM(code->co_varnames, i); // borrowed
						PyObject* def = kwdefaults != NULL ? PyDict_GetItem(kwdefaults, name) : NULL; // borrowed
						// TODO: ArgumentResolver
						PyPtr<> resolver = NewValueResolver(name, annots, def);
						if (resolver.IsNull() || PyDict_SetItem(kwargs, name, resolver) == -1) {
							return false;
						}
					}

					provider->kwargs = kwargs.Steal();
				}

				return true;
			}

			static inline bool Arguments(Provider* provider, PyObject* obj, int offset) {
				PyPtr<PyCodeObject> code = NULL;
				PyPtr<> callable = GetCallable(obj, code);
				// printf("Real Callable %s\n", ZenoDI_REPR(callable));

				if (callable.IsNull()) {
					return false;
				}

				// def defaults(arg1=1)
				PyPtr<> defaults = PyObject_GetAttr(callable, Module::State()->STR_DEFAULTS);
				if (defaults.IsNull()) {
					PyErr_Clear();
				} else if (defaults == Py_None) {
					defaults.Clear();
				} else {
					assert(PyTuple_CheckExact(defaults));
				}

				// def kwonly(*, kw1, kw2=2)
				PyPtr<> kwdefaults = NULL;
				if (code->co_kwonlyargcount) {
					kwdefaults = PyObject_GetAttr(callable, Module::State()->STR_KWDEFAULTS);
					if (kwdefaults.IsNull()) {
						PyErr_Clear();
					}
				}

				// def fn(arg1: SomeType)
				PyPtr<> annots = PyObject_GetAttr(callable, Module::State()->STR_ANNOTATIONS);
				ZenoDI_DUMP(annots);
				if (annots.IsNull()) {
					PyErr_Clear();
				}

				if (!offset) {
					PyPtr<> cself = PyObject_GetAttr(callable, Module::State()->STR_SELF);
					if (cself.IsNull()) {
						PyErr_Clear();
					} else {
						offset = 1;
					}
				}

				return Arguments(provider, code, defaults, kwdefaults, annots, offset);
			}
		} /* end namespace Callable */

		static inline bool Attributes(Provider* provider, PyObject* type) {
			PyPtr<> attributes = PyDict_New();
			if (attributes.IsNull()) {
				return false;
			}

			PyObject* mro = ((PyTypeObject*) type)->tp_mro;
			assert(mro != NULL);
			assert(PyTuple_CheckExact(mro));

			PyObject* str_annots = Module::State()->STR_ANNOTATIONS;
			for (Py_ssize_t i=PyTuple_GET_SIZE(mro) - 1; i>=0; i--) {
				PyObject* base = PyTuple_GET_ITEM(mro, i);
				assert(base != NULL);

				PyPtr<> annots = PyObject_GetAttr(base, str_annots);
				if (annots.IsNull()) {
					PyErr_Clear();
					continue;
				}
				assert(PyDict_CheckExact(annots));

				PyObject* key;
				PyObject* value;
				Py_ssize_t pos = 0;

				while (PyDict_Next(annots, &pos, &key, &value)) {
					// TODO: maybe default value???
					PyPtr<> resolver = (PyObject*) ValueResolver::New(key, value, NULL);
					if (PyDict_SetItem(attributes, key, resolver) == -1) {
						return false;
					}
				}
			}

			provider->attributes = attributes.Steal();
			return true;
		}
	} /* end namespace Collect */

	static inline PyObject* CreateOwnScope(PyObject* provide) {
		Py_RETURN_NONE;
	}

} // end namespace _provider


Provider* Provider::New(PyObject* value, Provider::Strategy strategy, PyObject* provide) {
	PyPtr<Provider> self = Provider::Alloc();
	if (self.IsNull()) {
		return NULL;
	}

	Py_INCREF(value);
	self->value = value;
	self->args = NULL;
	self->kwargs = NULL;
	self->attributes = NULL;
	self->own_scope = NULL;

	if (strategy & Provider::Strategy::FACTORY) {
		if (PyType_Check(value)) {
			self->value_type = Provider::ValueType::CLASS;
			if (!_provider::Collect::Attributes(self, value)) {
				return NULL;
			}

			PyPtr<> init = PyObject_GetAttr(value, Module::State()->STR_INIT);
			if (init.IsNull()) {
				return NULL;
			}

			// __init__ is optional
			if (!_provider::Collect::Callable::Arguments(self, init, 1)) {
				if (PyErr_ExceptionMatches(Module::State()->ExcProvideError)) {
					PyErr_Clear();
				} else {
					return NULL;
				}
			}
		} else {
			self->value_type = Provider::ValueType::FUNCTION;
			if (!_provider::Collect::Callable::Arguments(self, value, 0)) {
				return NULL;
			}
		}
	} else {
		self->value_type = Provider::ValueType::OTHER;
	}

	printf("args = %s\nkwargs = %s\nattrs = %s\n", ZenoDI_REPR(self->args), ZenoDI_REPR(self->kwargs), ZenoDI_REPR(self->attributes));

	if (provide != NULL) {
		if (self->value_type == Provider::ValueType::OTHER) {
			PyErr_SetString(Module::State()->ExcProvideError, ZenoDI_Err_GotProvideForValue);
			return NULL;
		} else {
			self->own_scope = _provider::CreateOwnScope(provide);
			if (self->own_scope == NULL) {
				return NULL;
			}
		}
	}


	return self.Steal();
}


Provider* Provider::New(PyObject* value, PyObject* strategy, PyObject* provide) {
	Provider::Strategy strat = Provider::Strategy::FACTORY;
	if (strategy != NULL) {
		if (!PyLong_Check(strategy)) {
			if (!PyCallable_Check(strategy)) {
				PyErr_Format(Module::State()->ExcProvideError, ZenoDI_Err_GotInvalidStrategy, strategy);
				return NULL;
			} else {
				strat = Provider::Strategy::CUSTOM | Provider::Strategy::FACTORY;
			}
		} else {
			strat = (Provider::Strategy) PyLong_AS_LONG(strategy);
			if (!(strat & Provider::Strategy::ALL)) {
				PyErr_Format(Module::State()->ExcProvideError, ZenoDI_Err_GotInvalidStrategyInt, strategy);
				return NULL;
			}
		}
	}

	return Provider::New(value, strat, provide);
}


PyObject* Provider::Resolve(Provider* self, Injector* injector) {
	Py_RETURN_NONE;
}


PyObject* Provider::__call__(Provider* self, PyObject* args, PyObject** kwargs) {
	// return Provider::Resolve(...)
	Py_RETURN_NONE;
}

void Provider::__dealloc__(Provider* self) {
	Py_CLEAR(self->value);
	Py_CLEAR(self->args);
	Py_CLEAR(self->kwargs);
	Py_CLEAR(self->attributes);
	Py_CLEAR(self->own_scope);
	Py_CLEAR(self->custom_strategy);
	Super::__dealloc__(self);
}

PyObject* Provider::bind(Provider* self, Injector* injector) {
	return (PyObject*) BoundProvider::New(self, injector);
}


BoundProvider* BoundProvider::New(Provider* provider, Injector* injector) {
	assert(Provider::CheckExact(provider));
	assert(Injector::CheckExact(injector));

	BoundProvider* self = BoundProvider::Alloc();
	if (self == NULL) { return NULL; }

	Py_INCREF(provider);
	Py_INCREF(injector);

	self->provider = provider;
	self->injector = injector;

	return self;
}


PyObject* BoundProvider::__call__(BoundProvider* self, PyObject* args, PyObject** kwargs) {
	//TODO: ha vannak paraméterek, akkor hiba
	assert(Provider::CheckExact(self->provider));
	assert(Injector::CheckExact(self->injector));

	return Provider::Resolve(self->provider, self->injector);
}


void BoundProvider::__dealloc__(BoundProvider* self) {
	Py_XDECREF(self->provider);
	Py_XDECREF(self->injector);
	Super::__dealloc__(self);
}


} // end namespace ZenoDI

#endif /* AC1E491D_0133_C8FB_12AA_F65E6B95E8A0 */
