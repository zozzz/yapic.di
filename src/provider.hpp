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

				// TODO: Refactor use PyFunctionObject* instead of getting attributes

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

	template<bool IsList>
	static inline bool InitOwnScope(Injector* injector, PyObject* provide) {
		Py_ssize_t len = (IsList ? PyList_GET_SIZE(provide) : PyTuple_GET_SIZE(provide));
		for (Py_ssize_t i=0; i<len ; i++) {
			PyObject* item = (IsList ? PyList_GET_ITEM(provide, i) : PyTuple_GET_ITEM(provide, i));
			assert(item != NULL);

			PyObject* provided;
			if (!PyTuple_CheckExact(item)) {
				provided = Injector::Provide(injector, item);
			} else {
				PyObject* id = NULL;
				PyObject* value = NULL;
				PyObject* strategy = NULL;
				PyObject* provide = NULL;
				if (PyArg_UnpackTuple(item, "provide", 1, 4, &id, &value, &strategy, &provide)) {
					provided = Injector::Provide(injector, id, value, strategy, provide);
				} else {
					return false;
				}
			}
			if (provided == NULL) {
				return false;
			} else {
				Py_DECREF(provided);
			}
		}
		return true;
	}

	static inline PyObject* CreateOwnScope(PyObject* provide) {
		PyPtr<Injector> injector = Injector::New(NULL);
		if (injector.IsNull()) {
			return NULL;
		}

		if (PyList_CheckExact(provide)) {
			if (!InitOwnScope<true>(injector, provide)) {
				return NULL;
			}
		} else if (PyTuple_CheckExact(provide)) {
			if (!InitOwnScope<false>(injector, provide)) {
				return NULL;
			}
		} else {
			PyErr_SetString(Module::State()->ExcProvideError, ZenoDI_Err_ProvideArgMustIterable);
			return NULL;
		}

		PyObject* scope = injector->scope;
		Py_INCREF(scope);
		return scope;
	}

	template<bool UseKwOnly>
	static inline PyObject* Factory(Provider* provider, Injector* injector) {
		PyPtr<> args = PyTuple_New((provider->args ? PyTuple_GET_SIZE(provider->args) : 0));
		if (args.IsNull()) {
			return NULL;
		}

		for (Py_ssize_t i = 0 ; i<PyTuple_GET_SIZE(args) ; i++) {
			ValueResolver* resolver = (ValueResolver*) PyTuple_GET_ITEM(provider->args, i);
			assert(ValueResolver::CheckExact(resolver));

			PyObject* arg = ValueResolver::Resolve<false>(resolver, injector);
			if (arg == NULL) {
				return NULL;
			}
			PyTuple_SET_ITEM(args, i, arg);
		}

		PyPtr<> kwargs = NULL;
		if (provider->kwargs) {
			kwargs = PyDict_New();
			if (kwargs.IsNull()) {
				return NULL;
			}

			PyObject* key;
			PyObject* value;
			Py_ssize_t pos = 0;

			while (PyDict_Next(provider->kwargs, &pos, &key, &value)) {
				PyObject* arg = ValueResolver::Resolve<true && UseKwOnly>((ValueResolver*) value, injector);
				if (arg == NULL) {
					return NULL;
				}
				if (PyDict_SetItem(kwargs, key, arg) == -1) {
					return NULL;
				}
			}
		}

		assert(provider->value != NULL);

		if (provider->value_type == Provider::ValueType::CLASS) {
			PyPtr<> __new__ = PyObject_GetAttr(provider->value, Module::State()->STR_NEW);
			if (__new__.IsNull()) {
				return NULL;
			}

			PyPtr<> newArgs = PyTuple_New(PyTuple_GET_SIZE(args) + 1);
			if (newArgs.IsNull()) {
				return NULL;
			}

			Py_INCREF(provider->value);
			PyTuple_SET_ITEM(newArgs, 0, provider->value);
			for (Py_ssize_t i = 0 ; i<PyTuple_GET_SIZE(args) ; i++) {
				PyObject* a = PyTuple_GET_ITEM(args, i);
				Py_INCREF(a);
				PyTuple_SET_ITEM(newArgs, i+1, a);
			}

			PyPtr<> obj = PyObject_Call(__new__, newArgs, kwargs);
			if (obj.IsNull()) {
				return NULL;
			}

			if (provider->attributes) {
				PyObject* key;
				PyObject* value;
				Py_ssize_t pos = 0;

				while (PyDict_Next(provider->attributes, &pos, &key, &value)) {
					PyObject* attr = ValueResolver::Resolve<false>((ValueResolver*) value, injector);
					if (attr == NULL) {
						return NULL;
					}
					if (PyObject_SetAttr(obj, key, attr) == -1) {
						return NULL;
					}
				}
			}

			PyPtr<> __init__ = PyObject_GetAttr(obj, Module::State()->STR_INIT);
			if (__init__.IsNull()) {
				return NULL;
			}
			if (PyObject_Call(__init__, args, kwargs) == NULL) {
				return NULL;
			}
			return obj.Steal();
		} else if (provider->value_type == Provider::ValueType::FUNCTION) {
			return PyObject_Call(provider->value, args, kwargs);
		} else {
			PyErr_BadInternalCall();
			return NULL;
		}
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
	self->custom_strategy = NULL;
	self->strategy = strategy;

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

	if (provide != NULL) {
		if (self->value_type == Provider::ValueType::OTHER || (self->strategy & Strategy::VALUE)) {
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
				Provider* provider = Provider::New(value, strat, provide);
				if (provider == NULL) {
					return NULL;
				}
				Py_INCREF(strategy);
				provider->custom_strategy = strategy;
				return provider;
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
	if (self->strategy & Strategy::VALUE) {
		Py_INCREF(self->value);
		return self->value;
	} else if (self->strategy & Strategy::FACTORY) {
		if (self->strategy & Strategy::SINGLETON) {
			if (self->strategy & Strategy::GLOBAL) {
				// lock begin
				// Module::State()->globals[self] ?= instance
				// lock end
			} else {
				// injector->scope[self] ?= instance
			}
		} else if (self->strategy & Strategy::CUSTOM) {
			assert(self->custom_strategy != NULL);
			PyPtr<ProviderFactory> factory = ProviderFactory::New(self, injector);
			if (factory.IsNull()) {
				return NULL;
			}
			return PyObject_CallFunctionObjArgs(self->custom_strategy, factory);
		} else {
			if (self->own_scope) {
				Injector* scope = Injector::New(injector, self->own_scope);
				if (scope == NULL) {
					return NULL;
				}
				return _provider::Factory<true>(self, scope);
			} else {
				return _provider::Factory<true>(self, injector);
			}
		}
	} else {
		PyErr_BadInternalCall();
		return NULL;
	}
}


PyObject* Provider::__call__(Provider* self, PyObject* args, PyObject** kwargs) {
	PyObject* injector;
	if (PyArg_UnpackTuple(args, "__call__", 1, 1, &injector)) {
		if (Injector::CheckExact(injector)) {
			return Provider::Resolve(self, (Injector*) injector);
		} else {
			PyErr_BadArgument();
			return NULL;
		}
	}
	return NULL;
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
	if (Injector::CheckExact(injector)) {
		return (PyObject*) BoundProvider::New(self, injector);
	} else {
		PyErr_BadArgument();
		return NULL;
	}
}


BoundProvider* BoundProvider::New(Provider* provider, Injector* injector) {
	assert(Provider::CheckExact(provider));
	assert(Injector::CheckExact(injector));

	BoundProvider* self = BoundProvider::Alloc();
	if (self == NULL) { return NULL; }

	assert(Provider::CheckExact(provider));
	assert(Injector::CheckExact(injector));

	Py_INCREF(provider);
	Py_INCREF(injector);

	self->provider = provider;
	self->injector = injector;

	return self;
}


PyObject* BoundProvider::__call__(BoundProvider* self, PyObject* args, PyObject** kwargs) {
	//TODO: ha vannak paraméterek, akkor hiba
	return Provider::Resolve(self->provider, self->injector);
}


void BoundProvider::__dealloc__(BoundProvider* self) {
	Py_XDECREF(self->provider);
	Py_XDECREF(self->injector);
	Super::__dealloc__(self);
}


ProviderFactory* ProviderFactory::New(Provider* provider, Injector* injector) {
	assert(Provider::CheckExact(provider));
	assert(Injector::CheckExact(injector));

	ProviderFactory* self = ProviderFactory::Alloc();
	if (self == NULL) { return NULL; }

	assert(Provider::CheckExact(provider));
	assert(Injector::CheckExact(injector));

	Py_INCREF(provider);
	Py_INCREF(injector);

	self->provider = provider;
	self->injector = injector;

	return self;
}


PyObject* ProviderFactory::__call__(ProviderFactory* self, PyObject* args, PyObject** kwargs) {
	//TODO: ha vannak paraméterek, akkor hiba

	if (self->provider->own_scope) {
		PyPtr<Injector> injector = Injector::New(self->injector, self->provider->own_scope);
		if (injector.IsNull()) {
			return NULL;
		}
		return _provider::Factory<true>(self->provider, injector);
	} else {
		return _provider::Factory<true>(self->provider, self->injector);
	}

}


void ProviderFactory::__dealloc__(ProviderFactory* self) {
	Py_XDECREF(self->provider);
	Py_XDECREF(self->injector);
	Super::__dealloc__(self);
}


} // end namespace ZenoDI

#endif /* AC1E491D_0133_C8FB_12AA_F65E6B95E8A0 */
