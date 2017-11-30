#ifndef AC1E491D_0133_C8FB_12AA_F65E6B95E8A0
#define AC1E491D_0133_C8FB_12AA_F65E6B95E8A0

#include "./di.hpp"

namespace ZenoDI {

namespace _injectable {
	static inline PyObject* NewVRFromID(Injectable* injectable, PyObject* name, PyObject* id, PyObject* defaultValue) {
		printf("NewVRFromID %s\n", ZenoDI_REPR(id));
		// if (PyUnicode_Check(id)) {
		// 	// TODO: ForwardRef resolver
		// } else {
		// 	printf("AAAAAAAAAAAAAAAAA\n");
		// 	PyPtr<> args = PyObject_GetAttr(id, Module::State()->STR_ARGS);
		// 	ZenoDI_DUMP(args);
		// 	if (args.IsNull()) {
		// 		// simple type, without Generics
		// 		PyErr_Clear();
		// 	} else {
		// 		// have generics like class X(Generic[T])
		// 		if (((PyObject*) args) != Py_None) {
		// 			PyPtr<> origin = PyObject_GetAttr(id, Module::State()->STR_ORIGIN);
		// 			if (origin.IsNull()) {
		// 				return NULL;
		// 			}
		// 			PyPtr<> params = PyObject_GetAttr(origin, Module::State()->STR_PARAMETERS);
		// 			if (params.IsNull()) {
		// 				return NULL;
		// 			}

		// 			ZenoDI_DUMP(origin);
		// 			ZenoDI_DUMP(params);
		// 		} else {

		// 		}
		// 	}
		// }


		return (PyObject*) ValueResolver::New(name, id, defaultValue);
	}

	static inline PyObject* NewVR(Injectable* injectable, PyObject* name, PyObject* annots, PyObject* defaultValue) {
		assert(name != NULL && PyUnicode_Check(name));
		assert(annots == NULL || PyDict_Check(annots));

		PyObject* id = annots == NULL ? NULL : PyDict_GetItem(annots, name); // borrowed ref, no exception
		return NewVRFromID(injectable, name, id, defaultValue);
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
			static inline bool GetFunction(PyObject* callable, PyFunctionObject*& func, PyObject*& self) {
				if (PyFunction_Check(callable)) {
					func = (PyFunctionObject*) callable;
					self = NULL;
					return true;
				} else if (PyMethod_Check(callable)) {
					func = (PyFunctionObject*) PyMethod_GET_FUNCTION(callable);
					assert(PyFunction_Check(func));
					self = PyMethod_GET_SELF(callable);
					return true;
				} else {
					/**
					 * class X:
					 *     def __call__(self): pass
					 */
					PyPtr<> __call__ = PyObject_GetAttr(callable, Module::State()->STR_CALL);
					if (__call__.IsValid()) {
						if (!PyObject_IsInstance(__call__, Module::State()->MethodWrapperType)) {
							return GetFunction(__call__, func, self);
						}
					} else {
						PyErr_Clear();
					}
				}

				// got builtin or any other unsupported callable object
				PyErr_Format(Module::State()->ExcProvideError, ZenoDI_Err_GotBuiltinCallable, callable);
				return NULL;
			}

			static inline bool Arguments(Injectable* injectable, PyObject* obj, int offset) {
				PyFunctionObject* func;
				PyObject* self;
				if (!GetFunction(obj, func, self)) {
					return false;
				}
				// printf("Real Function %s SELF %s\n", ZenoDI_REPR(func), ZenoDI_REPR(self));

				if (offset == 0 && self != NULL) {
					offset = 1;
				}

				PyCodeObject* code = (PyCodeObject*) PyFunction_GET_CODE(func);
				if (code != NULL) {
					assert(PyCode_Check(code));
				}

				PyObject* annots = PyFunction_GET_ANNOTATIONS(func);
				if (annots != NULL) {
					assert(PyDict_CheckExact(annots));
				}

				int argcount = code->co_argcount - offset;
				assert(argcount >= 0);
				if (argcount) {
					assert(PyTuple_CheckExact(code->co_varnames));
					assert(PyTuple_GET_SIZE(code->co_varnames) >= argcount);

					PyPtr<> args = PyTuple_New(argcount);
					if (args.IsNull()) {
						return false;
					}

					PyObject* defaults = PyFunction_GET_DEFAULTS(func);
					if (defaults == NULL) {
						for (int i=offset ; i<code->co_argcount ; ++i) {
							PyObject* resolver = NewVR(injectable, PyTuple_GET_ITEM(code->co_varnames, i), annots, NULL);
							if (resolver == NULL) {
								return false;
							}
							PyTuple_SET_ITEM(args, i - offset, resolver);
						}
					} else {
						assert(PyTuple_CheckExact(defaults));

						Py_ssize_t defcount = PyTuple_GET_SIZE(defaults);
						Py_ssize_t defcounter = 0;
						for (Py_ssize_t i=offset ; i<code->co_argcount ; ++i) {
							PyObject* def = (code->co_argcount - defcount <= i
								? PyTuple_GET_ITEM(defaults, defcounter++)
								: NULL);
							PyObject* resolver = NewVR(injectable, PyTuple_GET_ITEM(code->co_varnames, i), annots, def);
							if (resolver == NULL) {
								return false;
							}
							PyTuple_SET_ITEM(args, i - offset, resolver);
						}
					}
					injectable->args = args.Steal();
				}

				if (code->co_kwonlyargcount) {
					assert(PyTuple_GET_SIZE(code->co_varnames) >= code->co_kwonlyargcount + code->co_argcount);

					PyPtr<> kwargs = PyDict_New();
					if (kwargs.IsNull()) {
						return false;
					}

					PyObject* kwdefaults = PyFunction_GET_KW_DEFAULTS(func);
					if (kwdefaults == NULL) {
						for (int i=code->co_argcount ; i<code->co_kwonlyargcount + code->co_argcount ; i++) {
							PyObject* name = PyTuple_GET_ITEM(code->co_varnames, i); // borrowed
							PyPtr<> resolver = NewVR(injectable, name, annots, NULL);
							if (resolver.IsNull() || PyDict_SetItem(kwargs, name, resolver) == -1) {
								return false;
							}
						}
					} else {
						assert(PyDict_CheckExact(kwdefaults));

						for (int i=code->co_argcount ; i<code->co_kwonlyargcount + code->co_argcount ; i++) {
							PyObject* name = PyTuple_GET_ITEM(code->co_varnames, i); // borrowed
							PyObject* def = PyDict_GetItem(kwdefaults, name); // borrowed
							PyPtr<> resolver = NewVR(injectable, name, annots, def);
							if (resolver.IsNull() || PyDict_SetItem(kwargs, name, resolver) == -1) {
								return false;
							}
						}
					}

					injectable->kwargs = kwargs.Steal();
				}

				return true;
			}
		} /* end namespace Callable */

		static inline bool Attributes(Injectable* injectable, PyObject* type) {
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
					// TODO: skip ClassVar[...]
					// TODO: maybe default value???
					PyPtr<> resolver = (PyObject*) NewVRFromID(injectable, key, value, NULL);
					if (resolver.IsNull()) {
						return false;
					}
					if (PyDict_SetItem(attributes, key, resolver) == -1) {
						return false;
					}
				}
			}

			injectable->attributes = attributes.Steal();
			return true;
		}
	} /* end namespace Collect */

	template<bool IsList>
	static inline bool InitOwnInjector(Injector* injector, PyObject* provide) {
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

	static inline Injector* NewOwnInjector(PyObject* provide) {
		PyPtr<Injector> injector = Injector::New(NULL);
		if (injector.IsNull()) {
			return NULL;
		}

		if (PyList_CheckExact(provide)) {
			if (!InitOwnInjector<true>(injector, provide)) {
				return NULL;
			}
		} else if (PyTuple_CheckExact(provide)) {
			if (!InitOwnInjector<false>(injector, provide)) {
				return NULL;
			}
		} else {
			PyErr_SetString(Module::State()->ExcProvideError, ZenoDI_Err_ProvideArgMustIterable);
			return NULL;
		}

		return injector.Steal();
	}

	template<bool UseKwOnly>
	static inline PyObject* Factory(Injectable* injectable, Injector* injector, Injector* own_injector) {
		PyObject* tmp = injectable->args;
		Py_ssize_t argc = tmp == NULL ? 0 : PyTuple_GET_SIZE(tmp);
		PyPtr<> args = PyTuple_New(argc);
		if (args.IsNull()) {
			return NULL;
		}

		for (Py_ssize_t i = 0 ; i<argc ; i++) {
			ValueResolver* resolver = (ValueResolver*) PyTuple_GET_ITEM(tmp, i);
			assert(ValueResolver::CheckExact(resolver));

			PyObject* arg = ValueResolver::Resolve<false>(resolver, injector, own_injector);
			if (arg == NULL) {
				return NULL;
			}
			PyTuple_SET_ITEM(args, i, arg);
		}

		PyPtr<> kwargs = NULL;
		tmp = injectable->kwargs;
		if (tmp != NULL) {
			kwargs = PyDict_New();
			if (kwargs.IsNull()) {
				return NULL;
			}

			PyObject* key;
			PyObject* value;
			argc = 0;

			while (PyDict_Next(tmp, &argc, &key, &value)) {
				PyObject* arg = ValueResolver::Resolve<UseKwOnly>((ValueResolver*) value, injector, own_injector);
				if (arg == NULL || PyDict_SetItem(kwargs, key, arg) == -1) {
					return NULL;
				}
			}
		}

		tmp = injectable->value;
		assert(tmp != NULL);

		if (injectable->value_type == Injectable::ValueType::CLASS) {
			// TODO: kikukázni a Generic paramétereket az összes base osztályban, és provideolni

			PyTypeObject* type = (PyTypeObject*) tmp;
			newfunc __new__ = type->tp_new;
			if (__new__ == NULL) {
				PyErr_Format(PyExc_TypeError, "cannot create '%.100s' instances", type->tp_name);
				return NULL;
			}

			PyPtr<> obj = __new__(type, args, kwargs);
			if (obj.IsNull()) {
				return NULL;
			}

			// TODO: __origin__ kell a Generic-nél tesztelni
			// if (!PyType_IsSubtype(Py_TYPE(obj), type)) {
			// 	return obj.Steal();
			// }

			if (injectable->attributes) {
				assert(PyDict_CheckExact(injectable->attributes));

				PyObject* akey;
				PyObject* avalue;
				Py_ssize_t apos = 0;
				while (PyDict_Next(injectable->attributes, &apos, &akey, &avalue)) {
					assert(ValueResolver::CheckExact(avalue));
					PyPtr<> value = ValueResolver::Resolve<false>((ValueResolver*) avalue, injector, own_injector);
					if (value.IsNull()) {
						return NULL;
					}
					if (PyObject_SetAttr(obj, akey, value) < 0) {
						return NULL;
					}
				}
			}



			type = Py_TYPE(obj);
			if (type->tp_init != NULL) {
				int res = type->tp_init(obj, args, kwargs);
				if (res < 0) {
					return NULL;
				}
			}
			return obj.Steal();
		} else if (injectable->value_type == Injectable::ValueType::FUNCTION) {
			#if Py_DEBUG
				return PyObject_Call(tmp, args, kwargs);
			#else
				return Py_TYPE(tmp)->tp_call(tmp, args, kwargs);
			#endif
		} else {
			PyErr_BadInternalCall();
			return NULL;
		}
	}

} // end namespace _injectable


Injectable* Injectable::New(PyObject* value, Injectable::Strategy strategy, PyObject* provide) {
	PyPtr<Injectable> self = Injectable::Alloc();
	if (self.IsNull()) {
		return NULL;
	}

	Py_INCREF(value);
	self->value = value;
	self->args = NULL;
	self->kwargs = NULL;
	self->attributes = NULL;
	self->own_injector = NULL;
	self->custom_strategy = NULL;
	self->strategy = strategy;

	if (strategy & Injectable::Strategy::FACTORY) {
		if (PyType_Check(value)) {
			self->value_type = Injectable::ValueType::CLASS;
			if (!_injectable::Collect::Attributes(self, value)) {
				return NULL;
			}

			PyPtr<> init = PyObject_GetAttr(value, Module::State()->STR_INIT);
			if (init.IsNull()) {
				return NULL;
			}

			// printf("init %s ||| tp_init = %p\n", ZenoDI_REPR(init), Py_TYPE(value)->tp_init);

			// __init__ is optional
			if (!_injectable::Collect::Callable::Arguments(self, init, 1)) {
				if (PyErr_ExceptionMatches(Module::State()->ExcProvideError)) {
					PyErr_Clear();
				} else {
					return NULL;
				}
			}
		} else {
			self->value_type = Injectable::ValueType::FUNCTION;
			if (!_injectable::Collect::Callable::Arguments(self, value, 0)) {
				return NULL;
			}
		}
	} else {
		self->value_type = Injectable::ValueType::OTHER;
	}

	if (provide != NULL) {
		if (self->value_type == Injectable::ValueType::OTHER || (self->strategy & Strategy::VALUE)) {
			PyErr_SetString(Module::State()->ExcProvideError, ZenoDI_Err_GotProvideForValue);
			return NULL;
		} else if (self->own_injector == NULL) {
			self->own_injector = _injectable::NewOwnInjector(provide);
			if (self->own_injector == NULL) {
				return NULL;
			}
		}
	}

	return self.Steal();
}


Injectable* Injectable::New(PyObject* value, PyObject* strategy, PyObject* provide) {
	Injectable::Strategy strat = Injectable::Strategy::FACTORY;
	if (strategy != NULL) {
		if (!PyLong_Check(strategy)) {
			if (!PyCallable_Check(strategy)) {
				PyErr_Format(Module::State()->ExcProvideError, ZenoDI_Err_GotInvalidStrategy, strategy);
				return NULL;
			} else {
				strat = Injectable::Strategy::CUSTOM | Injectable::Strategy::FACTORY;
				Injectable* injectable = Injectable::New(value, strat, provide);
				if (injectable == NULL) {
					return NULL;
				}
				Py_INCREF(strategy);
				injectable->custom_strategy = strategy;
				return injectable;
			}
		} else {
			strat = (Injectable::Strategy) PyLong_AS_LONG(strategy);
			if (!(strat & Injectable::Strategy::ALL)) {
				PyErr_Format(Module::State()->ExcProvideError, ZenoDI_Err_GotInvalidStrategyInt, strategy);
				return NULL;
			}
		}
	}

	return Injectable::New(value, strat, provide);
}


PyObject* Injectable::Resolve(Injectable* self, Injector* injector) {
	auto strategy = self->strategy;
	if (strategy & Strategy::FACTORY) {
		if (strategy & Strategy::SINGLETON) {
			// lock begin
			if (strategy & Strategy::GLOBAL) {
				// Module::State()->globals[self] ?= instance
			} else {
				// injector->scope[self] ?= instance
			}
			// lock end
		} else if (self->custom_strategy != NULL) {
			PyPtr<> args = PyTuple_New(2);
			if (args.IsNull()) {
				return NULL;
			}
			Py_INCREF(self);
			Py_INCREF(injector);
			PyTuple_SET_ITEM(args, 0, (PyObject*) self);
			PyTuple_SET_ITEM(args, 1, (PyObject*) injector);
			return PyObject_Call(self->custom_strategy, args, NULL);
		} else {
			return _injectable::Factory<true>(self, injector, self->own_injector);
		}
	} else if (strategy & Strategy::VALUE) {
		Py_INCREF(self->value);
		return self->value;
	} else {
		PyErr_BadInternalCall();
		return NULL;
	}
}


PyObject* Injectable::__call__(Injectable* self, PyObject* args, PyObject** kwargs) {
	PyObject* injector;
	if (PyArg_UnpackTuple(args, "__call__", 1, 1, &injector)) {
		if (Injector::CheckExact(injector)) {
			return Injectable::Resolve(self, (Injector*) injector);
		} else {
			PyErr_BadArgument();
			return NULL;
		}
	}
	return NULL;
}

void Injectable::__dealloc__(Injectable* self) {
	Py_CLEAR(self->value);
	Py_CLEAR(self->args);
	Py_CLEAR(self->kwargs);
	Py_CLEAR(self->attributes);
	Py_CLEAR(self->own_injector);
	Py_CLEAR(self->custom_strategy);
	Super::__dealloc__(self);
}

PyObject* Injectable::bind(Injectable* self, Injector* injector) {
	if (Injector::CheckExact(injector)) {
		return (PyObject*) BoundInjectable::New(self, injector);
	} else {
		PyErr_BadArgument();
		return NULL;
	}
}


BoundInjectable* BoundInjectable::New(Injectable* injectable, Injector* injector) {
	assert(Injectable::CheckExact(injectable));
	assert(Injector::CheckExact(injector));

	BoundInjectable* self = BoundInjectable::Alloc();
	if (self == NULL) { return NULL; }

	assert(Injectable::CheckExact(injectable));
	assert(Injector::CheckExact(injector));

	Py_INCREF(injectable);
	Py_INCREF(injector);

	self->injectable = injectable;
	self->injector = injector;

	return self;
}


PyObject* BoundInjectable::__call__(BoundInjectable* self, PyObject* args, PyObject** kwargs) {
	//TODO: ha vannak paraméterek, akkor hiba
	return Injectable::Resolve(self->injectable, self->injector);
}


void BoundInjectable::__dealloc__(BoundInjectable* self) {
	Py_XDECREF(self->injectable);
	Py_XDECREF(self->injector);
	Super::__dealloc__(self);
}


} // end namespace ZenoDI

#endif /* AC1E491D_0133_C8FB_12AA_F65E6B95E8A0 */
