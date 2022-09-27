#ifndef AC1E491D_0133_C8FB_12AA_F65E6B95E8A0
#define AC1E491D_0133_C8FB_12AA_F65E6B95E8A0

#include "./di.hpp"


namespace YapicDI {

namespace _injectable {

	namespace ResolverFactory {
		static inline PyObject* NewResolver(PyObject* name, PyObject* type, PyObject* def) {
			PyPtr<> injectable(NULL);
			if (type != NULL && Module::State()->Typing.IsGenericType(type)) {
				injectable = (PyObject*)Injectable::New(type, Injectable::Strategy::FACTORY, (PyObject*)NULL);
				if (!injectable) {
					return NULL;
				}
			}

			return (PyObject*) ValueResolver::New(name, type, def, injectable);
		}

		static inline bool IsInjectable(PyObject* value) {
			if (Module::State()->Typing.IsGenericType(value)) {
				PyObject* origin = PyObject_GetAttr(value, Module::State()->STR_ORIGIN);
				if (origin == NULL) {
					PyErr_Clear();
					return false;
				}

				bool res = Module::State()->Inject.Eq(origin);
				Py_DECREF(origin);
				return res;
			} else {
				return false;
			}
		}

		static inline PyObject* GetInjectableAttr(PyObject* inject) {
			// Csak az ilyen formájó ForwardDecl osztályok érdekelnek: Inject["..."]
			if (Module::State()->Typing.IsForwardDecl(inject) && reinterpret_cast<Yapic::ForwardDecl*>(inject)->IsGeneric()) {
				// Yapic::ForwardDecl* forward = reinterpret_cast<Yapic::ForwardDecl*>(inject);
				PyPtr<PyTupleObject> unpacked = Module::State()->Typing.UnpackForwardDecl(inject);
				if (unpacked) {
					assert(PyTuple_CheckExact(unpacked));
					if (Module::State()->Inject.Eq(PyTuple_GET_ITEM(unpacked, 0))) {
						PyObject* args = PyTuple_GET_ITEM(unpacked, 1);
						assert(PyTuple_GET_SIZE(args) == 1);

						args = PyTuple_GET_ITEM(args, 0);
						Py_INCREF(args);
						return args;
					} else {
						return NULL;
					}
				} else {
					return NULL;
				}
			} else if (IsInjectable(inject)) {
				PyPtr<PyTupleObject> args = PyObject_GetAttr(inject, Module::State()->STR_ARGS);
				if (!args) {
					return NULL;
				}

				assert(PyTuple_CheckExact(args));
				assert(PyTuple_GET_SIZE(args) == 1);

				PyObject* res = PyTuple_GET_ITEM(args, 0);
				Py_INCREF(res);
				return res;
			} else {
				return NULL;
			}
		}

		template<bool KwOnly>
		static PyObject* Arguments(PyObject* hints) {
			Py_ssize_t l = PyTuple_GET_SIZE(hints);
			PyPtr<PyTupleObject> args = PyTuple_New(l);
			if (!args) {
				return NULL;
			}

			PyObject* entry;
			PyObject* argName;
			PyObject* argType;
			PyObject* argDef;
			PyObject* resolver;
			for (Py_ssize_t i = 0; i < l; ++i) {
				entry = PyTuple_GET_ITEM(hints, i);
				argName = PyTuple_GET_ITEM(entry, 0);
				argType = PyTuple_GET_ITEM(entry, 1);
				argDef = PyTuple_GET_SIZE(entry) >= 3
					? PyTuple_GET_ITEM(entry, 2)
					: NULL;

				if (argType == Py_None) {
					argType = NULL;
					if (!KwOnly && argDef == NULL) {
						PyErr_SetString(Module::State()->ExcProvideError, YapicDI_Err_CallableArgumentUntyped);
						return NULL;
					}
				}

				resolver = NewResolver(argName, argType, argDef);
				if (!resolver) {
					return NULL;
				}

				PyTuple_SET_ITEM(args, i, resolver);
			}

			return (PyObject*)args.Steal();
		}

		static bool Callable(Injectable* injectable, PyObject* hints) {
			PyObject* positional = PyTuple_GET_ITEM(hints, 0);
			if (positional != Py_None) {
				if ((injectable->args = Arguments<false>(positional)) == NULL) {
					return false;
				}
			}

			PyObject* kwOnly = PyTuple_GET_ITEM(hints, 1);
			if (kwOnly != Py_None) {
				if ((injectable->kwargs = Arguments<true>(kwOnly)) == NULL) {
					return false;
				}
			}

			return true;
		}

		static bool Attribute(Injectable* injectable, PyObject* hints) {
			Py_ssize_t l = PyDict_Size(hints);
			Py_ssize_t attrsSize = 0;
			PyPtr<PyTupleObject> attrs = PyTuple_New(l);
			if (!attrs) {
				return false;
			}

			Py_ssize_t pos = 0;
			PyObject* key;
			PyObject* value;
			PyObject* resolver;
			PyPtr<> attrType(NULL);

			while (PyDict_Next(hints, &pos, &key, &value)) {
				attrType = GetInjectableAttr(value);
				if (attrType) {
					resolver = NewResolver(key, attrType, NULL);
					if (resolver != NULL) {
						PyTuple_SET_ITEM(attrs, attrsSize, resolver);
						++attrsSize;
					} else {
						return false;
					}
				} else if (PyErr_Occurred()) {
					return false;
				} else {
					continue;
				}
			}

			injectable->attributes = (PyObject*)attrs.Steal();
			if (_PyTuple_Resize(&injectable->attributes, attrsSize) == -1) {
				return false;
			} else {
				return true;
			}
		}

	} /* end namespace Resolver */

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
			PyErr_SetString(Module::State()->ExcProvideError, YapicDI_Err_ProvideArgMustIterable);
			return NULL;
		}

		return injector.Steal();
	}

	template<typename Value>
	struct Strategy_None {
		static PyObject* Get(Injectable* self, Injector* injector, Injector* owni, int recursion) {
			return Value::Get(self, injector, owni, recursion);
		};
	};


	template<typename Value>
	struct Strategy_Singleton {
		static PyObject* Get(Injectable* self, Injector* injector, Injector* owni, int recursion) {
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
		static PyObject* Get(Injectable* self, Injector* injector, Injector* owni, int recursion) {
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
		static PyObject* Get(Injectable* self, Injector* injector, Injector* owni, int recursion) {
			assert(self->resolved != NULL);
			assert(PyCallable_Check(self->resolved));
			return PyObject_CallFunctionObjArgs(self->resolved, self, injector, NULL);
		};
	};


	template<typename Invoker>
	struct Value_Invoke {
		static FORCEINLINE PyObject* Get(Injectable* self, Injector* injector, Injector* owni, int recursion) {
			if (++recursion >= YapicDI_MAX_RECURSION) {
				PyErr_Format(PyExc_RecursionError, YapicDI_Err_RecursionError, self);
				return NULL;
			}
			return Invoker::Invoke(self, injector, owni, recursion);
		}
	};


	struct Value_Const {
		static FORCEINLINE PyObject* Get(Injectable* self, Injector* injector, Injector* owni, int recursion) {
			Py_INCREF(self->value);
			return self->value;
		}
	};


	template<bool AllowKwOnly, bool HasKwOnly>
	struct InvokeFn {
		static FORCEINLINE PyObject* Invoke(Injectable* self, Injector* injector, Injector* owni, int recursion) {
			PyObject* args = InvokeFn<AllowKwOnly, HasKwOnly>::PosArgs::New(self, injector, owni, recursion);
			PyObject* kwargs = NULL;
			PyObject* res = NULL;
			if (args == NULL) {
				return NULL;
			}

			if (HasKwOnly) {
				kwargs = InvokeFn<AllowKwOnly, HasKwOnly>::KwArgs::New(self, injector, owni, recursion);
				if (kwargs == NULL) {
					Py_DECREF(args);
					return NULL;
				} else if (kwargs == Py_None) {
					kwargs = NULL;
				}
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

		struct Positional {
			static constexpr bool _AllowKwOnly = false;

			static FORCEINLINE PyObject* Resolvers(Injectable* injectable) {
				return injectable->args;
			}

			static FORCEINLINE PyObject* New(Py_ssize_t size) {
				return PyTuple_New(size);
			}

			static FORCEINLINE bool Set(PyObject* args, Py_ssize_t i, ValueResolver* vr, PyObject* value) {
				Py_INCREF(value);
				PyTuple_SET_ITEM(args, i, value);
				return true;
			}
		};

		struct Keyword {
			static constexpr bool _AllowKwOnly = AllowKwOnly;

			static FORCEINLINE PyObject* Resolvers(Injectable* injectable) {
				return injectable->kwargs;
			}

			static FORCEINLINE PyObject* New(Py_ssize_t size) {
				return _PyDict_NewPresized(size);
			}

			static FORCEINLINE bool Set(PyObject* args, Py_ssize_t i, ValueResolver* vr, PyObject* value) {
				return PyDict_SetItem(args, vr->name, value) == 0;
			}
		};

		template<typename Trait>
		struct CallArgs {
			static FORCEINLINE PyObject* New(Injectable* self, Injector* injector, Injector* owni, int recursion) {
				PyObject* resolvers = Trait::Resolvers(self);
				PyObject* result;

				if (resolvers == NULL) {
					return Trait::New(0);
				}

				assert(PyTuple_CheckExact(resolvers));

				Py_ssize_t size = PyTuple_GET_SIZE(resolvers);
				result = Trait::New(size);
				if (result != NULL) {
					ValueResolver* resolver;
					PyObject* resolved;

					for (Py_ssize_t i = 0; i < size; ++i) {
						resolver = reinterpret_cast<ValueResolver*>(PyTuple_GET_ITEM(resolvers, i));
						assert(resolver != NULL && ValueResolver::CheckExact(resolver));

						resolved = ValueResolver::Resolve<Trait::_AllowKwOnly>(resolver, injector, owni, recursion);
						if (resolved != NULL) {
							bool res = Trait::Set(result, i, resolver, resolved);
							Py_DECREF(resolved);
							if (!res) {
								goto error;
							}
						} else {
							goto error;
						}
					}

					return result;
				} else {
					return NULL;
				}

				error:
					Py_DECREF(result);
					return NULL;
			};
		};

		using PosArgs = InvokeFn<AllowKwOnly, HasKwOnly>::CallArgs<Positional>;
		using KwArgs = InvokeFn<AllowKwOnly, HasKwOnly>::CallArgs<Keyword>;
	};

	template<bool AllowKwOnly, bool HasArgs, bool HasKwOnly>
	struct InvokeClass {
		static FORCEINLINE PyObject* Invoke(Injectable* self, Injector* injector, Injector* owni, int recursion) {
			PyPtr<> args = HasArgs
				? InvokeFn<AllowKwOnly, HasKwOnly>::PosArgs::New(self, injector, owni, recursion)
				: PyTuple_New(0);
			if (args.IsNull()) {
				return NULL;
			}

			PyPtr<> kwargs = NULL;
			if (HasKwOnly) {
				kwargs = InvokeFn<AllowKwOnly, HasKwOnly>::KwArgs::New(self, injector, owni, recursion);
				if (kwargs.IsNull()) {
					return NULL;
				} else if (kwargs.As<PyObject>() == Py_None) {
					kwargs = NULL;
				}
			}

			PyTypeObject* type = (PyTypeObject*) self->value;
			assert(type->tp_new != NULL);
			PyObject* obj = type->tp_new(type, args, kwargs);
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

				assert(objType->tp_init);
				if (objType->tp_init(obj, args, kwargs) < 0) {
					goto error;
				}

				return obj;
				error:
					Py_DECREF(obj);
					return NULL;
			}
			return NULL;
		}

		static FORCEINLINE bool SetAttributes(Injectable* self, Injector* injector, Injector* owni, int recursion, PyObject* obj) {
			PyObject* attrs = self->attributes;
			if (attrs != NULL) {
				assert(PyTuple_CheckExact(attrs));

				Py_ssize_t l = PyTuple_GET_SIZE(attrs);
				ValueResolver* vr;
				PyObject* value;

				for (Py_ssize_t i = 0; i < l; ++i) {
					vr = (ValueResolver*)PyTuple_GET_ITEM(attrs, i);
					assert(vr != NULL && ValueResolver::CheckExact(vr));
					value = ValueResolver::Resolve<false>(vr, injector, owni, recursion);
					if (value != NULL) {
						bool succ = PyObject_GenericSetAttr(obj, vr->name, value) == 0;
						Py_DECREF(value);
						if (!succ) {
							return false;
						}
					} else {
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
		return NULL;
	};

	template<bool HasArgs, bool HasKwOnly>
	using ClassValue = Value_Invoke<InvokeClass<true, HasArgs, HasKwOnly>>;
	template<bool HasKwOnly>
	using FunctionValue = Value_Invoke<InvokeFn<true, HasKwOnly>>;
	using KwOnlyGetter = Value_Invoke<InvokeFn<false, true>>;
	using BasicValue = Value_Const;

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

	if (strategy != Injectable::Strategy::VALUE) {
		if (PyType_Check(value) || Module::State()->Typing.IsGenericType(value)) {
			PyPtr<PyTupleObject> hints = Module::State()->Typing.TypeHints(value);
			if (!hints) {
				return NULL;
			}

			value = PyTuple_GET_ITEM(hints, 0);
			Py_INCREF(value);
			Py_DECREF(self->value);
			self->value = value;

			PyObject* attrs = PyTuple_GET_ITEM(hints, 1);
			if (attrs != Py_None) {
				if (!_injectable::ResolverFactory::Attribute(self, attrs)) {
					return NULL;
				}
			}

			PyObject* init = PyTuple_GET_ITEM(hints, 2);
			if (init != Py_None) {
				if (!_injectable::ResolverFactory::Callable(self, init)) {
					return NULL;
				}
			}

			if (self->kwargs) {
				if (self->args) {
					self->strategy = _injectable::GetStrategy<_injectable::ClassValue<true, true>>(strategy);
					self->get_value = &_injectable::ClassValue<true, true>::Get;
				} else {
					self->strategy = _injectable::GetStrategy<_injectable::ClassValue<false, true>>(strategy);
					self->get_value = &_injectable::ClassValue<false, true>::Get;
				}
			} else {
				if (self->args) {
					self->strategy = _injectable::GetStrategy<_injectable::ClassValue<true, false>>(strategy);
					self->get_value = &_injectable::ClassValue<true, false>::Get;
				} else {
					self->strategy = _injectable::GetStrategy<_injectable::ClassValue<false, false>>(strategy);
					self->get_value = &_injectable::ClassValue<false, false>::Get;
				}
			}
		} else {
			PyPtr<> hints = Module::State()->Typing.CallableHints(value);
			if (!hints) {
				return NULL;
			}

			if (!_injectable::ResolverFactory::Callable(self, hints)) {
				return NULL;
			}

			if (self->kwargs) {
				self->strategy = _injectable::GetStrategy<_injectable::FunctionValue<true>>(strategy);
				self->get_value = &_injectable::FunctionValue<true>::Get;
			} else {
				self->strategy = _injectable::GetStrategy<_injectable::FunctionValue<false>>(strategy);
				self->get_value = &_injectable::FunctionValue<false>::Get;
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
		self->get_value = &_injectable::BasicValue::Get;

		if (provide != NULL) {
			PyErr_SetString(Module::State()->ExcProvideError, YapicDI_Err_GotProvideForValue);
			return NULL;
		}
	}

	assert(self->strategy);
	assert(self->get_value);
	return self.Steal();
}


Injectable* Injectable::New(PyObject* value, PyObject* strategy, PyObject* provide) {
	Injectable::Strategy strat = Injectable::Strategy::FACTORY;
	if (strategy != NULL) {
		if (!PyLong_Check(strategy)) {
			if (!PyCallable_Check(strategy)) {
				PyErr_Format(Module::State()->ExcProvideError, YapicDI_Err_GotInvalidStrategy, strategy);
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
			if (strat <= 0 || strat > YapicDI_Injectable_Strategy_MAX) {
				PyErr_Format(Module::State()->ExcProvideError, YapicDI_Err_GotInvalidStrategyInt, strategy);
				return NULL;
			}
		}
	}

	return Injectable::New(value, strat, provide);
}


PyObject* Injectable::Resolve(Injectable* self, Injector* injector, int recursion) {
	assert(self != NULL && Injectable::CheckExact(self));
	assert(injector != NULL);
	return self->strategy(self, injector, self->own_injector, recursion);
}


PyObject* Injectable::__new__(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
	static char *kwlist[] = {"value", "strategy", "provide", NULL};

	PyObject* value = NULL;
	PyObject* strategy = NULL;
	PyObject* provide = NULL;

	if (PyArg_ParseTupleAndKeywords(args, kwargs, "O|OO:Injectable", kwlist, &value, &strategy, &provide)) {
		return (PyObject*)Injectable::New(value, strategy, provide);
	} else {
		return NULL;
	}
}


PyObject* Injectable::__call__(Injectable* self, PyObject* args, PyObject** kwargs) {
	assert(args != NULL);
	if (PyTuple_CheckExact(args) && PyTuple_GET_SIZE(args) == 1) {
		Injector* injector = (Injector*) PyTuple_GET_ITEM(args, 0);
		if (Injector::CheckExact(injector)) {
			return self->get_value(self, injector, self->own_injector, 0);
		}
	}
	PyErr_SetString(PyExc_TypeError, YapicDI_Err_OneInjectorArg);
	return NULL;
}


PyObject* Injectable::bind(Injectable* self, Injector* injector) {
	if (Injector::CheckExact(injector)) {
		return (PyObject*) BoundInjectable::New(self, injector, self->hash);
	} else {
		PyErr_SetString(PyExc_TypeError, YapicDI_Err_OneInjectorArg);
		return NULL;
	}
}


PyObject* Injectable::resolve(Injectable* self, Injector* injector) {
	if (injector != NULL && Injector::CheckExact(injector)) {
		return Injectable::Resolve(self, (Injector*) injector, 0);
	} else {
		PyErr_SetString(PyExc_TypeError, YapicDI_Err_OneInjectorArg);
		return NULL;
	}
}


ValueResolver* Injectable::GetPosArg(Injectable* self, Py_ssize_t index) {
	if (self->args != NULL && PyTuple_GET_SIZE(self->args) > index) {
		assert(ValueResolver::CheckExact(PyTuple_GET_ITEM(self->args, index)));
		return (ValueResolver*)PyTuple_GET_ITEM(self->args, index);
	} else {
		return NULL;
	}
}

ValueResolver* Injectable::GetKwArg(Injectable* self, PyObject* name) {
	if (self->kwargs != NULL) {
		Py_ssize_t l = PyTuple_GET_SIZE(self->kwargs);

		ValueResolver* cv;
		for (Py_ssize_t i = 0; i < l; ++i) {
			cv = (ValueResolver*)PyTuple_GET_ITEM(self->kwargs, i);
			assert(ValueResolver::CheckExact(cv));
			assert(cv->name != NULL);

			switch (PyObject_RichCompareBool(cv->name, name, Py_EQ)) {
				case 1: return cv;
				case 0: continue;
				case -1: PyErr_Clear(); continue;
			}
		}
	}
	return NULL;
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


int Injectable::__traverse__(Injectable* self, visitproc visit, void *arg) {
	Py_VISIT(self->value);
	Py_VISIT(self->args);
	Py_VISIT(self->kwargs);
	Py_VISIT(self->attributes);
	Py_VISIT(self->own_injector);
	Py_VISIT(self->resolved);
	return 0;
}


int Injectable::__clear__(Injectable* self) {
	PyObject_GC_UnTrack(self);
	Py_CLEAR(self->value);
	Py_CLEAR(self->args);
	Py_CLEAR(self->kwargs);
	Py_CLEAR(self->attributes);
	Py_CLEAR(self->own_injector);
	Py_CLEAR(self->resolved);
	return 0;
}


void Injectable::__dealloc__(Injectable* self) {
	Py_CLEAR(self->value);
	Py_CLEAR(self->args);
	Py_CLEAR(self->kwargs);
	Py_CLEAR(self->attributes);
	Py_CLEAR(self->own_injector);
	Py_CLEAR(self->resolved);
	Py_TYPE(self)->tp_free((PyObject*)self);
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
	Py_TYPE(self)->tp_free((PyObject*)self);
}


#define YapicDI_HashMethods(__cls) \
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


YapicDI_HashMethods(Injectable)
YapicDI_HashMethods(BoundInjectable)

#undef YapicDI_HashMethods

} // end namespace YapicDI

#endif /* AC1E491D_0133_C8FB_12AA_F65E6B95E8A0 */
