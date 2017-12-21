#ifndef AC1E491D_0133_C8FB_12AA_F65E6B95E8A0
#define AC1E491D_0133_C8FB_12AA_F65E6B95E8A0

#include "./di.hpp"
#include "./util.hpp"


namespace ZenoDI {

namespace _injectable {
	static inline PyObject* NewVRFromID(PyObject* name, PyObject* id, PyObject* def, PyObject* aliases, PyObject* g) {
		if (aliases != NULL) {
			PyObject* alias = PyDict_GetItem(aliases, id); // borrowed, no exception
			if (alias != NULL) {
				id = alias;
			}
		}
		return (PyObject*) ValueResolver::New(name, id, def, g);
	}

	static inline PyObject* NewVR(PyObject* name, PyObject* annots, PyObject* def, PyObject* aliases, PyObject* g) {
		assert(name != NULL && PyUnicode_Check(name));
		assert(annots == NULL || PyDict_Check(annots));

		PyObject* id = annots == NULL ? NULL : PyDict_GetItem(annots, name); // borrowed ref, no exception
		return NewVRFromID(name, id, def, aliases, g);
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

			static inline bool Arguments(Injectable* injectable, PyObject* obj, PyObject* aliases, int offset) {
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

				PyObject* globals = PyFunction_GET_GLOBALS(func);
				if (globals != NULL) {
					assert(PyDict_CheckExact(globals));
				}

				PyObject* defaults;
				int argcount = code->co_argcount - offset;
				assert(argcount >= 0);
				if (argcount) {
					assert(PyTuple_CheckExact(code->co_varnames));
					assert(PyTuple_GET_SIZE(code->co_varnames) >= argcount);

					PyPtr<> args = PyTuple_New(argcount);
					if (args.IsNull()) {
						return false;
					}

					defaults = PyFunction_GET_DEFAULTS(func);
					if (defaults == NULL) {
						for (int i=offset ; i<code->co_argcount ; ++i) {
							PyObject* resolver = NewVR(
								PyTuple_GET_ITEM(code->co_varnames, i), annots, NULL, aliases, globals);
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
							PyObject* resolver = NewVR(
								PyTuple_GET_ITEM(code->co_varnames, i), annots, def, aliases, globals);
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

					defaults = PyFunction_GET_KW_DEFAULTS(func);
					if (defaults == NULL) {
						for (int i=code->co_argcount ; i<code->co_kwonlyargcount + code->co_argcount ; i++) {
							PyObject* name = PyTuple_GET_ITEM(code->co_varnames, i); // borrowed
							PyPtr<> resolver = NewVR(name, annots, NULL, aliases, globals);
							if (resolver.IsNull() || PyDict_SetItem(kwargs, name, resolver) == -1) {
								return false;
							}
						}
					} else {
						assert(PyDict_CheckExact(defaults));

						for (int i=code->co_argcount ; i<code->co_kwonlyargcount + code->co_argcount ; i++) {
							PyObject* name = PyTuple_GET_ITEM(code->co_varnames, i); // borrowed
							PyObject* def = PyDict_GetItem(defaults, name); // borrowed
							PyPtr<> resolver = NewVR(name, annots, def, aliases, globals);
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

		namespace Type {
			static inline bool Attributes(Injectable* injectable, PyTypeObject* type, PyObject* typeAliases) {
				PyPtr<> attributes = PyDict_New();
				if (attributes.IsNull()) {
					return false;
				}

				PyObject* mro = type->tp_mro;
				assert(mro != NULL);
				assert(PyTuple_CheckExact(mro));

				PyObject* str_annots = Module::State()->STR_ANNOTATIONS;
				PyObject* str_module = Module::State()->STR_MODULE;
				for (Py_ssize_t i=PyTuple_GET_SIZE(mro) - 1; i>=0; i--) {
					PyObject* base = PyTuple_GET_ITEM(mro, i);
					assert(base != NULL);

					PyPtr<> annots = PyObject_GetAttr(base, str_annots);
					if (annots.IsNull()) {
						PyErr_Clear();
						continue;
					}
					assert(PyDict_CheckExact(annots));

					if (PyDict_Size(annots) == 0) {
						continue;
					}

					// TODO: found better ide to get class definier injectables
					PyPtr<> module = PyObject_GetAttr(base, str_module);
					if (module.IsNull()) {
						return false;
					}
					module = ImportModule(module);
					if (module.IsNull()) {
						return false;
					}

					PyObject* key;
					PyObject* value;
					Py_ssize_t pos = 0;

					while (PyDict_Next(annots, &pos, &key, &value)) {
						// TODO: skip ClassVar[...]
						// TODO: maybe default value???
						PyPtr<> resolver = (PyObject*) NewVRFromID(key, value, NULL, typeAliases, module);
						if (resolver.IsNull()) {
							return false;
						}
						if (PyDict_SetItem(attributes, key, resolver) == -1) {
							return false;
						}
					}
				}

				if (PyDict_Size(attributes) > 0) {
					injectable->attributes = attributes.Steal();
				}

				return true;
			}
		} /* end namespace Type */
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

	// TODO: kiprobálni a PyObject_CallFunctionObjArgs
	// template<bool UseKwOnly>
	// static inline PyObject* Factory(Injectable* injectable, Injector* injector, Injector* own_injector, int recursion) {
	// 	if (++recursion >= ZenoDI_MAX_RECURSION) {
	// 		PyErr_Format(PyExc_RecursionError, ZenoDI_Err_RecursionError, injectable);
	// 		return NULL;
	// 	}

	// 	PyObject* tmp = injectable->args;
	// 	Py_ssize_t argc = tmp == NULL ? 0 : PyTuple_GET_SIZE(tmp);
	// 	PyPtr<> args = PyTuple_New(argc);
	// 	if (args.IsNull()) {
	// 		return NULL;
	// 	}

	// 	for (Py_ssize_t i = 0 ; i<argc ; i++) {
	// 		ValueResolver* resolver = (ValueResolver*) PyTuple_GET_ITEM(tmp, i);
	// 		assert(ValueResolver::CheckExact(resolver));

	// 		PyObject* arg = ValueResolver::Resolve<false>(resolver, injector, own_injector, recursion);
	// 		if (arg == NULL) {
	// 			return NULL;
	// 		}
	// 		PyTuple_SET_ITEM(args, i, arg);
	// 	}

	// 	PyPtr<> kwargs = NULL;
	// 	tmp = injectable->kwargs;
	// 	if (tmp != NULL) {
	// 		kwargs = PyDict_New();
	// 		if (kwargs.IsNull()) {
	// 			return NULL;
	// 		}

	// 		PyObject* key;
	// 		PyObject* value;
	// 		argc = 0;

	// 		while (PyDict_Next(tmp, &argc, &key, &value)) {
	// 			PyObject* arg = ValueResolver::Resolve<UseKwOnly>((ValueResolver*) value, injector, own_injector, recursion);
	// 			if (arg == NULL || PyDict_SetItem(kwargs, key, arg) == -1) {
	// 				return NULL;
	// 			}
	// 		}
	// 	}

	// 	tmp = injectable->value;
	// 	assert(tmp != NULL);

	// 	if (injectable->value_type == Injectable::ValueType::CLASS) {
	// 		PyTypeObject* type = (PyTypeObject*) tmp;
	// 		newfunc __new__ = type->tp_new;
	// 		if (__new__ == NULL) {
	// 			PyErr_Format(PyExc_TypeError, "cannot create '%.100s' instances", type->tp_name);
	// 			return NULL;
	// 		}

	// 		PyPtr<> obj = __new__(type, args, kwargs);
	// 		if (obj.IsNull()) {
	// 			return NULL;
	// 		}

	// 		PyTypeObject* objType = Py_TYPE(obj);
	// 		if (!PyType_IsSubtype(objType, type)) {
	// 			PyObject* mro = type->tp_mro;
	// 			assert(mro != NULL);
	// 			assert(PyTuple_CheckExact(mro));

	// 			if (PyTuple_GET_SIZE(mro) <= 1 ||
	// 				!PyType_IsSubtype(objType, (PyTypeObject*) PyTuple_GET_ITEM(mro, 1))) {
	// 				return obj.Steal();
	// 			}
	// 		}

	// 		if (injectable->attributes) {
	// 			assert(PyDict_CheckExact(injectable->attributes));

	// 			PyObject* akey;
	// 			PyObject* avalue;
	// 			Py_ssize_t apos = 0;
	// 			while (PyDict_Next(injectable->attributes, &apos, &akey, &avalue)) {
	// 				assert(ValueResolver::CheckExact(avalue));
	// 				PyPtr<> value = ValueResolver::Resolve<false>((ValueResolver*) avalue, injector, own_injector, recursion);
	// 				if (value.IsNull()) {
	// 					return NULL;
	// 				}
	// 				if (PyObject_SetAttr(obj, akey, value) < 0) {
	// 					return NULL;
	// 				}
	// 			}
	// 		}

	// 		if (objType->tp_init != NULL) {
	// 			int res = objType->tp_init(obj, args, kwargs);
	// 			if (res < 0) {
	// 				return NULL;
	// 			}
	// 		}
	// 		return obj.Steal();
	// 	} else {
	// 		assert(injectable->value_type == Injectable::ValueType::FUNCTION);
	// 		#ifdef Py_DEBUG
	// 			return PyObject_Call(tmp, args, kwargs);
	// 		#else
	// 			return Py_TYPE(tmp)->tp_call(tmp, args, kwargs);
	// 		#endif
	// 	}
	// }

	// static inline PyObject* SingletonFactory(Injectable* self, Injector* injector, PyObject* singletons, int recursion) {
	// 	PyObject* inst = _PyDict_GetItem_KnownHash(singletons, (PyObject*) self, self->hash);
	// 	if (inst != NULL) {
	// 		Py_INCREF(inst);
	// 		return inst;
	// 	} else {
	// 		PyErr_Clear();
	// 	}

	// 	inst = _injectable::Factory<true>(self, injector, self->own_injector, recursion);
	// 	if (inst != NULL && _PyDict_SetItem_KnownHash(singletons, (PyObject*) self, inst, self->hash) < 0) {
	// 		Py_DECREF(inst);
	// 		return NULL;
	// 	}
	// 	return inst;
	// }

	// namespace __xxx {

		// Strategy_None
		// Strategy_Singleton
		// Strategy_Scoped

		// singleton fn = Strategy_Singleton<Value_Invoke<InvokeFn<true>>>



		template<typename Value>
		struct Strategy_None {
			static inline PyObject* Get(Injectable* self, Injector* injector, Injector* owni, int recursion) {
				return Value::Get(self, injector, owni, recursion);
			};
		};


		template<typename Value>
		struct Strategy_Singleton {
			static inline PyObject* Get(Injectable* self, Injector* injector, Injector* owni, int recursion) {
				PyObject* resolved = self->resolved;
				if (resolved != NULL) {
					Py_INCREF(resolved);
				} else {
					resolved = Value::Get(self, injector, owni, recursion);
					Py_XINCREF(resolved);
					self->resolved = resolved;
				}
				return resolved;
			};
		};


		template<typename Value>
		struct Strategy_Scoped {
			static inline PyObject* Get(Injectable* self, Injector* injector, Injector* owni, int recursion) {
				PyObject* singletons = injector->singletons;
				PyObject* inst = _PyDict_GetItem_KnownHash(singletons, (PyObject*) self, self->hash);
				if (inst != NULL) {
					Py_INCREF(inst);
					return inst;
				} else {
					PyErr_Clear();
				}

				inst = Value::Get(self, injector, owni, recursion);
				if (inst != NULL && _PyDict_SetItem_KnownHash(singletons, (PyObject*) self, inst, self->hash) < 0) {
					Py_DECREF(inst);
					return NULL;
				}
				return inst;
			};
		};


		template<typename Value>
		struct Strategy_Custom {
			static inline PyObject* Get(Injectable* self, Injector* injector, Injector* owni, int recursion) {
				assert(self->resolved != NULL);
				assert(PyCallable_Check(self->resolved));
				return PyObject_CallFunctionObjArgs(self->resolved, self, injector, NULL);
				// return Value::Get(self, injector, owni, recursion);
			};
		};


		template<typename Invoker>
		struct Value_Invoke {
			static inline PyObject* Get(Injectable* self, Injector* injector, Injector* owni, int recursion) {
				if (++recursion >= ZenoDI_MAX_RECURSION) {
					PyErr_Format(PyExc_RecursionError, ZenoDI_Err_RecursionError, self);
					return NULL;
				}
				return Invoker::Invoke(self, injector, owni, recursion);
			}
		};


		struct Value_Const {
			static inline PyObject* Get(Injectable* self, Injector* injector, Injector* owni, int recursion) {
				Py_INCREF(self->value);
				return self->value;
			}
		};


		template<bool AllowKwOnly>
		struct InvokeFn {
			static inline PyObject* Invoke(Injectable* self, Injector* injector, Injector* owni, int recursion) {
				PyObject* args = InvokeFn<AllowKwOnly>::GetCallArgs(self, injector, owni, recursion);
				PyObject* kwargs = NULL;
				PyObject* res = NULL;
				if (args == NULL) {
					return NULL;
				}

				kwargs = InvokeFn<AllowKwOnly>::GetCallKwargs(self, injector, owni, recursion);
				if (kwargs == NULL) {
					Py_DECREF(args);
					return NULL;
				} else if (kwargs == Py_None) {
					kwargs = NULL;
				}

				#ifdef Py_DEBUG
					res = PyObject_Call(self->value, args, kwargs);
				#else
					PyObject* value = self->value;
					res = Py_TYPE(value)->tp_call(value, args, kwargs);
				#endif
				Py_DECREF(args);
				Py_XDECREF(kwargs);
				return res;
			};

			static inline PyObject* GetCallArgs(Injectable* self, Injector* injector, Injector* owni, int recursion) {
				register PyObject* tmp = self->args;
				Py_ssize_t argc = tmp == NULL ? 0 : PyTuple_GET_SIZE(tmp);
				PyObject* args = PyTuple_New(argc);
				if (args != NULL) {
					for (Py_ssize_t i = 0 ; i<argc ; i++) {
						ValueResolver* resolver = (ValueResolver*) PyTuple_GET_ITEM(tmp, i);
						assert(ValueResolver::CheckExact(resolver));

						PyObject* arg = ValueResolver::Resolve<false>(resolver, injector, owni, recursion);
						if (arg == NULL) {
							goto error;
						}
						PyTuple_SET_ITEM(args, i, arg);
					}
					return args;
					error:
						Py_DECREF(args);
						return NULL;
				}
				return Py_None;
			}

			static inline PyObject* GetCallKwargs(Injectable* self, Injector* injector, Injector* owni, int recursion) {
				PyObject* kwargs = self->kwargs;

				if (kwargs != NULL) {
					PyObject* result = PyDict_New();
					PyObject* key;
					PyObject* value;
					Py_ssize_t pos = 0;

					if (result == NULL) {
						return NULL;
					}

					while (PyDict_Next(kwargs, &pos, &key, &value)) {
						PyObject* arg = ValueResolver::Resolve<AllowKwOnly>((ValueResolver*) value, injector, owni, recursion);
						if (arg == NULL || PyDict_SetItem(kwargs, key, arg) < 0) {
							goto error;
						}
					}

					return result;
					error:
						Py_XDECREF(result);
						return NULL;
				}
				return NULL;
			}
		};


		template<bool AllowKwOnly>
		struct InvokeClass {
			static inline PyObject* Invoke(Injectable* self, Injector* injector, Injector* owni, int recursion) {
				PyTypeObject* type = (PyTypeObject*) self->value;
				newfunc __new__ = type->tp_new;
				if (__new__ == NULL) {
					PyErr_Format(PyExc_TypeError, "cannot create '%.100s' instances", type->tp_name);
					return NULL;
				}

				PyObject* args = InvokeFn<AllowKwOnly>::GetCallArgs(self, injector, owni, recursion);
				PyObject* kwargs = NULL;
				if (args == NULL) {
					return NULL;
				}

				kwargs = InvokeFn<AllowKwOnly>::GetCallKwargs(self, injector, owni, recursion);
				if (kwargs == NULL) {
					Py_DECREF(args);
					return NULL;
				} else if (kwargs == Py_None) {
					kwargs = NULL;
				}

				PyObject* obj = __new__(type, args, kwargs);
				if (obj != NULL) {
					PyTypeObject* objType = Py_TYPE(obj);

					if (!PyType_IsSubtype(objType, type)) {
						PyObject* mro = type->tp_mro;
						assert(mro != NULL);
						assert(PyTuple_CheckExact(mro));

						if (PyTuple_GET_SIZE(mro) <= 1 ||
							!PyType_IsSubtype(objType, (PyTypeObject*) PyTuple_GET_ITEM(mro, 1))) {
							return obj;
						}
					}

					if (!SetAttributes(self, injector, owni, recursion, obj)) {
						goto error;
					}

					if (objType->tp_init != NULL) {
						int res = objType->tp_init(obj, args, kwargs);
						if (res < 0) {
							goto error;
						}
					}

					return obj;
					error:
						Py_DECREF(obj);
						return NULL;
				}
				return NULL;
			}

			static inline bool SetAttributes(Injectable* self, Injector* injector, Injector* owni, int recursion, PyObject* obj) {
				PyObject* attrs = self->attributes;
				if (attrs != NULL) {
					assert(PyDict_CheckExact(attrs));

					PyObject* akey;
					PyObject* avalue;
					PyObject* value;
					Py_ssize_t apos = 0;
					while (PyDict_Next(attrs, &apos, &akey, &avalue)) {
						assert(ValueResolver::CheckExact(avalue));
						value = ValueResolver::Resolve<false>((ValueResolver*) avalue, injector, owni, recursion);
						if (value != NULL && PyObject_GenericSetAttr(obj, akey, value) < 0) {
							Py_XDECREF(value);
							return false;
						}
					}
				}
				return true;
			}
		};


		template<typename Value>
		static inline Injectable::StrategyCallback GetStrategy(Injectable::Strategy strategy) {
			switch (strategy) {
				case Injectable::Strategy::FACTORY:
					return &Strategy_None<Value>::Get;

				case Injectable::Strategy::SINGLETON:
					return &Strategy_Singleton<Value>::Get;

				case Injectable::Strategy::SCOPED:
					return &Strategy_Scoped<Value>::Get;

				case Injectable::Strategy::CUSTOM:
					return &Strategy_Custom<Value>::Get;

				case Injectable::Strategy::VALUE:
					return &Strategy_None<Value>::Get;
			}
		};

		using ClassValue = Value_Invoke<InvokeClass<true>>;
		using FunctionValue = Value_Invoke<InvokeClass<true>>;
		using BasicValue = Value_Invoke<InvokeClass<true>>;

	// } /* end namespace __xxx */


	// static PyObject* sfactory(Injectable* self, Injector* injector, int recursion) {
	// 	return _injectable::Factory<true>(self, injector, self->own_injector, recursion);
	// }

	// static PyObject* ssingleton(Injectable* self, Injector* injector, int recursion) {
	// 	Yapic::RLock::Auto lock(Module::State()->rlock_singletons);
	// 	return _injectable::SingletonFactory(self, injector, Module::State()->singletons, recursion);
	// }

	// static PyObject* sscoped(Injectable* self, Injector* injector, int recursion) {
	// 	return _injectable::SingletonFactory(self, injector, injector->singletons, recursion);
	// }

	// static PyObject* svalue(Injectable* self, Injector* injector, int recursion) {
	// 	Py_INCREF(self->value);
	// 	return self->value;
	// }

	// static PyObject* scustom(Injectable* self, Injector* injector, int recursion) {
	// 	assert(self->custom_strategy != NULL);
	// 	return PyObject_CallFunctionObjArgs(self->custom_strategy, self, injector, NULL);
	// }

	// static const Injectable::StrategyCallback strategy_callbacks[] = {
	// 	NULL,
	// 	&sfactory,
	// 	&ssingleton,
	// 	&sscoped,
	// 	&scustom,
	// 	&svalue
	// };

} // end namespace _injectable


Injectable* Injectable::New(PyObject* value, Injectable::Strategy strategy, PyObject* provide) {
	PyPtr<Injectable> self = Injectable::Alloc();
	if (self.IsNull()) {
		return NULL;
	}

	Py_INCREF(value);
	self->value = value;
	self->resolved = NULL;
	self->args = NULL;
	self->kwargs = NULL;
	self->attributes = NULL;
	self->own_injector = NULL;
	// self->custom_strategy = NULL;
	// self->strategy = strategy;

	if (strategy != Injectable::Strategy::VALUE) {
		if (PyType_Check(value)) {
			self->strategy = _injectable::GetStrategy<_injectable::ClassValue>(strategy);

			PyPtr<> typeAliases = ResolveTypeVars(value);
			if (typeAliases.IsNull() && PyErr_Occurred()) {
				return NULL;
			}

			if (!_injectable::Collect::Type::Attributes(self, (PyTypeObject*) value, typeAliases)) {
				return NULL;
			}

			PyPtr<> init = PyObject_GetAttr(value, Module::State()->STR_INIT);
			if (init.IsNull()) {
				return NULL;
			}

			// __init__ is optional
			if (!_injectable::Collect::Callable::Arguments(self, init, typeAliases, 1)) {
				if (PyErr_ExceptionMatches(Module::State()->ExcProvideError)) {
					PyErr_Clear();
				} else {
					return NULL;
				}
			}
		} else {
			self->strategy = _injectable::GetStrategy<_injectable::FunctionValue>(strategy);
			if (!_injectable::Collect::Callable::Arguments(self, value, NULL, 0)) {
				return NULL;
			}
		}

		if (provide != NULL) {
			self->own_injector = _injectable::NewOwnInjector(provide);
			if (self->own_injector == NULL) {
				return NULL;
			}
		}
	} else {
		self->strategy = _injectable::GetStrategy<_injectable::BasicValue>(Injectable::Strategy::VALUE);

		if (provide != NULL) {
			PyErr_SetString(Module::State()->ExcProvideError, ZenoDI_Err_GotProvideForValue);
			return NULL;
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
				strat = Injectable::Strategy::CUSTOM;
				Injectable* injectable = Injectable::New(value, strat, provide);
				if (injectable == NULL) {
					return NULL;
				}
				Py_INCREF(strategy);
				injectable->resolved = strategy;
				return injectable;
			}
		} else {
			strat = (Injectable::Strategy) PyLong_AS_LONG(strategy);
			if (strat <= 0 || strat > Injectable::Strategy::MAX) {
				PyErr_Format(Module::State()->ExcProvideError, ZenoDI_Err_GotInvalidStrategyInt, strategy);
				return NULL;
			}
		}
	}

	return Injectable::New(value, strat, provide);
}


PyObject* Injectable::Resolve(Injectable* self, Injector* injector, int recursion) {
	return self->strategy(self, injector, self->own_injector, recursion);
	// assert(self->strategy < Injectable::Strategy::MAX);
	// return _injectable::strategy_callbacks[self->strategy](self, injector, recursion);

	// switch (self->strategy) {
	// 	case Injectable::Strategy::FACTORY:
	// 		return _injectable::Factory<true>(self, injector, self->own_injector, recursion);

	// 	// case Injectable::Strategy::SINGLETON: {
	// 	// 	Yapic::RLock::Auto lock(Module::State()->rlock_singletons);
	// 	// 	return _injectable::SingletonFactory(self, injector, Module::State()->singletons, recursion);
	// 	// }

	// 	case Injectable::Strategy::SINGLETON: {
	// 		PyObject* res = self->resolved;
	// 		if (res != NULL) {
	// 			Py_INCREF(res);
	// 			return res;
	// 		} else {
	// 			res = _injectable::Factory<true>(self, injector, self->own_injector, recursion);
	// 			Py_XINCREF(res);
	// 			self->resolved = res;
	// 			return res;
	// 		}
	// 	}

	// 		// Yapic::RLock::Auto lock(Module::State()->rlock_singletons);
	// 		// return _injectable::SingletonFactory(self, injector, Module::State()->singletons, recursion);


	// 	case Injectable::Strategy::SCOPED:
	// 		return _injectable::SingletonFactory(self, injector, injector->singletons, recursion);

	// 	case Injectable::Strategy::CUSTOM:
	// 		assert(self->custom_strategy != NULL);
	// 		return PyObject_CallFunctionObjArgs(self->custom_strategy, self, injector, NULL);

	// 	case Injectable::Strategy::VALUE:
	// 		Py_INCREF(self->value);
	// 		return self->value;

	// 	default:
	// 		assert(0);
	// 		return NULL;
	// }
}


PyObject* Injectable::__call__(Injectable* self, PyObject* args, PyObject** kwargs) {
	// if (self->strategy != Injectable::Strategy::VALUE) {
		assert(args != NULL);
		if (PyTuple_CheckExact(args) && PyTuple_GET_SIZE(args) == 1) {
			Injector* injector = (Injector*) PyTuple_GET_ITEM(args, 0);
			if (Injector::CheckExact(injector)) {
				Py_RETURN_NONE; // TODO: úgy meghívni a factoryt, hogy nincs felhasználva a stratégia
				// return _injectable::Factory<true>(self, injector, self->own_injector, 0);
			}
		}
		PyErr_SetString(PyExc_TypeError, ZenoDI_Err_OneInjectorArg);
		return NULL;
	// } else {
	// 	PyErr_SetString(Module::State()->ExcInjectError, ZenoDI_Err_NotFactory);
	// 	return NULL;
	// }
}


PyObject* Injectable::bind(Injectable* self, Injector* injector) {
	if (Injector::CheckExact(injector)) {
		return (PyObject*) BoundInjectable::New(self, injector, self->hash);
	} else {
		PyErr_SetString(PyExc_TypeError, ZenoDI_Err_OneInjectorArg);
		return NULL;
	}
}


PyObject* Injectable::resolve(Injectable* self, Injector* injector) {
	if (injector != NULL && Injector::CheckExact(injector)) {
		return Injectable::Resolve(self, (Injector*) injector, 0);
	} else {
		PyErr_SetString(PyExc_TypeError, ZenoDI_Err_OneInjectorArg);
		return NULL;
	}
}


#define Injectable_AppendIndent(level) { \
		int __l=(level)*4;\
		if (!builder->EnsureSize(__l)) { \
			return false; \
		} \
		for (int i=0 ; i<__l ; ++i) { \
			builder->AppendAscii(' '); \
		} \
	}

#define Injectable_AppendString(x) \
	if (!builder->AppendStringSafe(x)) { \
		return false; \
	}

bool Injectable::ToString(Injectable* self, UnicodeBuilder* builder, int level) {
	Injectable_AppendIndent(level);
	Injectable_AppendString("<Injectable");

	// if (self->value_type == Injectable::ValueType::OTHER) {
	// 	if (!builder->AppendStringSafe(" VALUE ")) {
	// 		return false;
	// 	}
	// 	PyPtr<> repr = PyObject_Repr(self->value);
	// 	Injectable_AppendString(repr);
	// 	Injectable_AppendString(">");
	// 	return true;
	// }

	// if (self->value_type == Injectable::ValueType::CLASS) {
	// 	Injectable_AppendString(" CLASS ");
	// 	PyPtr<> repr = PyObject_Repr(self->value);
	// 	if (repr.IsNull()) {
	// 		return false;
	// 	}
	// 	const char* str = (const char*) PyUnicode_1BYTE_DATA(repr.As<PyASCIIObject>());
	// 	if (!builder->AppendStringSafe(str + 7, PyUnicode_GET_LENGTH(repr.As<PyASCIIObject>()) - 7 - 1)) {
	// 		return false;
	// 	}
	// 	if (self->attributes) {
	// 		PyObject* k;
	// 		PyObject* v;
	// 		Py_ssize_t p=0;

	// 		Injectable_AppendString("\n");
	// 		Injectable_AppendIndent(level + 1);
	// 		Injectable_AppendString("Attributes:");

	// 		while (PyDict_Next(self->attributes, &p, &k, &v)) {
	// 			Injectable_AppendString("\n");
	// 			Injectable_AppendIndent(level + 2);
	// 			if (PyUnicode_CheckExact(k)) {
	// 				if (!builder->AppendStringSafe(k)) {
	// 					return false;
	// 				}
	// 			} else {
	// 				Injectable_AppendString("<NOT STRING>");
	// 			}
	// 			Injectable_AppendString(": ");
	// 			PyPtr<> vr = PyObject_Repr(v);
	// 			if (vr.IsNull()) {
	// 				return false;
	// 			}
	// 			Injectable_AppendString(vr);
	// 		}
	// 	}
	// 	if (self->args || self->kwargs) {
	// 		Injectable_AppendString("\n");
	// 		Injectable_AppendIndent(level + 1);
	// 		Injectable_AppendString("__init__:");
	// 	}
	// } else if (self->value_type == Injectable::ValueType::FUNCTION) {
	// 	if (!builder->AppendStringSafe(" FUNCTION ")) {
	// 		return false;
	// 	}
	// 	PyPtr<> repr = PyObject_Repr(self->value);
	// 	if (repr.IsNull()) {
	// 		return false;
	// 	}
	// 	Injectable_AppendString(repr);
	// 	if (self->args || self->kwargs) {
	// 		Injectable_AppendString("\n");
	// 		Injectable_AppendIndent(level + 1);
	// 		Injectable_AppendString("Params:");
	// 	}
	// }

	// if (self->args) {
	// 	for (Py_ssize_t i=0 ; i<PyTuple_GET_SIZE(self->args) ; ++i) {
	// 		Injectable_AppendString("\n");
	// 		Injectable_AppendIndent(level + 2);
	// 		PyPtr<> repr = PyObject_Repr(PyTuple_GET_ITEM(self->args, i));
	// 		if (repr.IsNull()) {
	// 			return NULL;
	// 		}
	// 		Injectable_AppendString(repr);
	// 	}
	// }

	// if (self->kwargs) {
	// 	Injectable_AppendString("\n");
	// 	Injectable_AppendIndent(level + 2);
	// 	Injectable_AppendString("KwOnly:");
	// 	PyObject* k;
	// 	PyObject* v;
	// 	Py_ssize_t p=0;
	// 	while (PyDict_Next(self->kwargs, &p, &k, &v)) {
	// 		Injectable_AppendString("\n");
	// 		Injectable_AppendIndent(level + 3);
	// 		Injectable_AppendString(k);
	// 		Injectable_AppendString(": ");
	// 		PyPtr<> repr = PyObject_Repr(v);
	// 		if (repr.IsNull()) {
	// 			return NULL;
	// 		}
	// 		Injectable_AppendString(repr);
	// 	}
	// }

	// if (self->own_injector) {
	// 	Injectable_AppendString("\n");
	// 	Injectable_AppendIndent(level + 1);
	// 	Injectable_AppendString("OwnInjector:");

	// 	if (self->own_injector->kwargs) {
	// 		Injectable_AppendString("\n");
	// 		Injectable_AppendIndent(level + 2);
	// 		Injectable_AppendString("KwOnly:");
	// 		PyObject* kwonly = self->own_injector->kwargs;
	// 		for (Py_ssize_t i=0 ; i<PyList_GET_SIZE(kwonly) ; ++i) {
	// 			Injectable_AppendString("\n");
	// 			KwOnly* kwonlyItem = (KwOnly*) PyList_GET_ITEM(kwonly, i);
	// 			assert(Injectable::CheckExact(kwonlyItem->getter));
	// 			Injectable::ToString(kwonlyItem->getter, builder, level + 3);
	// 		}
	// 	}

	// 	if (self->own_injector->injectables && PyDict_Size(self->own_injector->injectables) > 0) {
	// 		Injectable_AppendString("\n");
	// 		Injectable_AppendIndent(level + 2);
	// 		Injectable_AppendString("Injectables:");
	// 		PyObject* k;
	// 		PyObject* v;
	// 		Py_ssize_t p=0;
	// 		while (PyDict_Next(self->own_injector->injectables, &p, &k, &v)) {
	// 			Injectable_AppendString("\n");
	// 			if (!Injectable::ToString((Injectable*) v, builder, level + 3)) {
	// 				return false;
	// 			}
	// 		}
	// 	}
	// }

	Injectable_AppendString(">");

	return true;
}


PyObject* Injectable::__repr__(Injectable* self) {
	UnicodeBuilder builder;
	if (Injectable::ToString(self, &builder, 0)) {
		return builder.ToPython();
	}
	return NULL;
}


void Injectable::__dealloc__(Injectable* self) {
	Py_CLEAR(self->value);
	Py_CLEAR(self->resolved);
	Py_CLEAR(self->args);
	Py_CLEAR(self->kwargs);
	Py_CLEAR(self->attributes);
	Py_CLEAR(self->own_injector);
	// Py_CLEAR(self->custom_strategy);
	Super::__dealloc__(self);
}


BoundInjectable* BoundInjectable::New(Injectable* injectable, Injector* injector, Py_hash_t hash) {
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
	self->hash = hash;

	return self;
}


PyObject* BoundInjectable::__call__(BoundInjectable* self, PyObject* args, PyObject** kwargs) {
	if (PyTuple_GET_SIZE(args) > 0) {
		PyErr_Format(PyExc_TypeError, "__call__ expected 0 arguments, got %i", PyTuple_GET_SIZE(args));
		return NULL;
	}
	return Injectable::Resolve(self->injectable, self->injector, 0);
}


void BoundInjectable::__dealloc__(BoundInjectable* self) {
	Py_XDECREF(self->injectable);
	Py_XDECREF(self->injector);
	Super::__dealloc__(self);
}


// InjectableFactory* InjectableFactory::New(Injectable* injectable, Injector* injector, Py_hash_t hash) {
// 	assert(Injectable::CheckExact(injectable));
// 	assert(Injector::CheckExact(injector));

// 	InjectableFactory* self = InjectableFactory::Alloc();
// 	if (self == NULL) { return NULL; }

// 	assert(Injectable::CheckExact(injectable));
// 	assert(Injector::CheckExact(injector));

// 	Py_INCREF(injectable);
// 	Py_INCREF(injector);

// 	self->injectable = injectable;
// 	self->injector = injector;
// 	self->hash = hash;

// 	return self;
// }


// PyObject* InjectableFactory::__call__(InjectableFactory* self, PyObject* args, PyObject** kwargs) {
// 	return _injectable::Factory<true>(self->injectable, self->injector, self->injectable->own_injector, 0);
// }


// void InjectableFactory::__dealloc__(InjectableFactory* self) {
// 	Py_XDECREF(self->injectable);
// 	Py_XDECREF(self->injector);
// 	Allocator::Dealloc(self);
// }


/*

PyObject* res; \
		switch (op) { \
			case Py_LT: res = (self->hash < otherHash ? Py_True : Py_False); break; \
			case Py_LE: res = (self->hash <= otherHash ? Py_True : Py_False); break; \
			case Py_EQ: res = (self->hash == otherHash ? Py_True : Py_False); break; \
			case Py_NE: res = (self->hash != otherHash ? Py_True : Py_False); break; \
			case Py_GT: res = (self->hash > otherHash ? Py_True : Py_False); break; \
			case Py_GE: res = (self->hash >= otherHash ? Py_True : Py_False); break; \
		} \
		Py_INCREF(res); \
		return res; \

*/

#define ZenoDI_HashMethods(__cls) \
	Py_hash_t __cls::__hash__(__cls* self) { \
		return self->hash; \
	} \
	PyObject* __cls::__cmp__(__cls* self, PyObject* other, int op) { \
		if (op == Py_EQ && __cls::CheckExact(other)) { \
			if (self->hash == ((__cls*) other)->hash) { \
				Py_RETURN_TRUE; \
			} else { \
				Py_RETURN_FALSE; \
			} \
		} \
		Py_RETURN_FALSE; \
	}


ZenoDI_HashMethods(Injectable)
ZenoDI_HashMethods(BoundInjectable)
// ZenoDI_HashMethods(InjectableFactory)

#undef ZenoDI_HashMethods

} // end namespace ZenoDI

#endif /* AC1E491D_0133_C8FB_12AA_F65E6B95E8A0 */
