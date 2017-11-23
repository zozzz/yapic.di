#ifndef AC1E491D_0133_C8FB_12AA_F65E6B95E8A0
#define AC1E491D_0133_C8FB_12AA_F65E6B95E8A0

#include "./module.hpp"

namespace ZenoDI {

namespace _provider {
	static inline PyObject* CreateValueResolver(PyObject* name, PyObject* annots, PyObject* defaultValue) {
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
			static inline PyObject* GetCallable(PyObject* callable, PyCodeObject** code) {
				assert(*code == NULL);

				*code = (PyCodeObject*) PyObject_GetAttr(callable, ModuleState::Get()->str__code__);
				if (*code == NULL) {
					PyErr_Clear();
					/**
					 * class X:
					 *     def __call__(self): pass
					 */
					PyObject* __call__ = PyObject_GetAttr(callable, ModuleState::Get()->str__call__);
					if (__call__ != NULL) {
						PyObject* res = NULL;
						if (PyObject_IsInstance(__call__, ModuleState::Get()->MethodWrapperType)) {
							// got builtin or any other unsupported callable object
							PyErr_Format(ModuleState::Get()->ExcProvideError, ZenoDI_Err_GotBuiltinCallable, callable);
						} else {
							res = GetCallable(__call__, code);
						}
						Py_DECREF(__call__);
						return res;
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
				PyErr_Format(ModuleState::Get()->ExcProvideError, ZenoDI_Err_GotBuiltinCallable, callable);
				return NULL;
			}

			static inline bool Arguments(Provider* provider, PyCodeObject* code,
										 PyObject* defaults, PyObject* kwdefaults, PyObject* annots, int offset) {
				printf("-------------------------\n");
				PyObject* args = NULL;
				PyObject* kwargs = NULL;
				int argcount = code->co_argcount - offset;
				assert(argcount >= 0);

				args = PyTuple_New(argcount);
				if (args == NULL) {
					goto error;
				}

				if (argcount) {
					assert(PyTuple_CheckExact(code->co_varnames));

					int defcount = (defaults == NULL ? 0 : (int) PyTuple_GET_SIZE(defaults));
					int defcounter = 0;
					for (int i=offset ; i<code->co_argcount ; ++i) {
						assert(PyTuple_GET_SIZE(code->co_varnames) > i);

						PyObject* def = (defcount && code->co_argcount - defcount <= i
							? PyTuple_GET_ITEM(defaults, defcounter++)
							: NULL);
						PyObject* resolver = CreateValueResolver(PyTuple_GET_ITEM(code->co_varnames, i), annots, def);
						if (resolver == NULL) {
							goto error;
						}
						PyTuple_SET_ITEM(args, i - offset, resolver);
					}
				}

				if (code->co_kwonlyargcount) {
					kwargs = PyDict_New();
					if (kwargs == NULL) {
						goto error;
					}

					for (int i=code->co_argcount ; i<code->co_kwonlyargcount + code->co_argcount ; i++) {
						assert(PyTuple_GET_SIZE(code->co_varnames) > i);
						PyObject* name = PyTuple_GET_ITEM(code->co_varnames, i); // borrowed
						PyObject* def = kwdefaults != NULL ? PyDict_GetItem(kwdefaults, name) : NULL; // borrowed
						PyObject* resolver = CreateValueResolver(name, annots, def);
						if (resolver == NULL) {
							goto error;
						} else if (PyDict_SetItem(kwargs, name, resolver) == -1) {
							Py_DECREF(resolver);
							goto error;
						} else {
							Py_DECREF(resolver);
						}
					}
				}

				provider->args = args;
				provider->kwargs = kwargs;

				return true;
				error:
					Py_XDECREF(args);
					Py_XDECREF(kwargs);
					return false;
			}

			static inline bool Arguments(Provider* provider, PyObject* obj, int offset) {
				PyCodeObject* code = NULL;
				PyObject* callable = GetCallable(obj, &code);
				// printf("Real Callable %s\n", ZenoDI_REPR(callable));

				if (callable == NULL) {
					Py_XDECREF(code);
					return false;
				}

				// def defaults(arg1=1)
				PyObject* defaults = PyObject_GetAttr(callable, ModuleState::Get()->str__defaults__);
				if (defaults == NULL) {
					PyErr_Clear();
				} else if (defaults == Py_None) {
					Py_CLEAR(defaults);
				} else {
					assert(PyTuple_CheckExact(defaults));
				}

				// def kwonly(*, kw1, kw2=2)
				PyObject* kwdefaults = NULL;
				if (code->co_kwonlyargcount) {
					kwdefaults = PyObject_GetAttr(callable, ModuleState::Get()->str__kwdefaults__);
					if (kwdefaults == NULL) {
						PyErr_Clear();
					}
				}

				// def fn(arg1: SomeType)
				PyObject* annots = PyObject_GetAttr(callable, ModuleState::Get()->str__annotations__);
				if (annots == NULL) {
					PyErr_Clear();
				}

				if (!offset) {
					PyObject* cself = PyObject_GetAttr(callable, ModuleState::Get()->str__self__);
					if (cself == NULL) {
						PyErr_Clear();
					} else {
						offset = 1;
						Py_DECREF(cself);
					}
				}

				bool res = Arguments(provider, code, defaults, kwdefaults, annots, offset);
				Py_DECREF(code);
				Py_XDECREF(defaults);
				Py_XDECREF(kwdefaults);
				Py_XDECREF(annots);
				return res;
			}
		} /* end namespace Callable */

		static inline bool Attributes(Provider* provider, PyObject* type) {
			return true;
		}
	} /* end namespace Collect */

	static inline PyObject* CreateOwnScope(PyObject* provide) {
		Py_RETURN_NONE;
	}

} // end namespace _provider


Provider* Provider::New(PyObject* value, Provider::Strategy strategy, PyObject* provide) {
	Provider* self = Provider::Alloc();
	if (self == NULL) {
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

			PyObject* init = PyObject_GetAttr(value, ModuleState::Get()->str__init__);
			if (init == NULL) {
				return NULL;
			}

			bool result = _provider::Collect::Callable::Arguments(self, init, 1);
			Py_DECREF(init);
			// __init__ is optional
			if (!result) {
				if (PyErr_ExceptionMatches(ModuleState::Get()->ExcProvideError)) {
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

	printf("args = %s\nkwargs = %s\n", ZenoDI_REPR(self->args), ZenoDI_REPR(self->kwargs), ZenoDI_REPR(provide));

	if (provide != NULL) {
		if (self->value_type == Provider::ValueType::OTHER) {
			PyErr_SetString(ModuleState::Get()->ExcProvideError, ZenoDI_Err_GotProvideForValue);
			return NULL;
		} else {
			self->own_scope = _provider::CreateOwnScope(provide);
			if (self->own_scope == NULL) {
				return NULL;
			}
		}
	}

	return self;
}

Provider* Provider::New(PyObject* value, PyObject* strategy, PyObject* provide) {
	Provider::Strategy strat = Provider::Strategy::FACTORY;
	if (strategy != NULL) {
		if (!PyLong_Check(strategy)) {
			if (!PyCallable_Check(strategy)) {
				PyErr_Format(ModuleState::Get()->ExcProvideError, ZenoDI_Err_GotInvalidStrategy, strategy);
				return NULL;
			} else {
				strat = Provider::Strategy::CUSTOM | Provider::Strategy::FACTORY;
			}
		} else {
			strat = (Provider::Strategy) PyLong_AS_LONG(strategy);
			if (!(strat & Provider::Strategy::ALL)) {
				PyErr_Format(ModuleState::Get()->ExcProvideError, ZenoDI_Err_GotInvalidStrategyInt, strategy);
				return NULL;
			}
		}
	}

	return Provider::New(value, strat, provide);
}

PyObject* Provider::Resolve(Provider* self, Injector* injector) {
	Py_RETURN_NONE;
}

PyObject* Provider::Exec(Provider* self, Injector* injector) {
	Py_RETURN_NONE;
}


ZenoDI_PrivateNew(Provider)


PyTypeObject ZenoDI_TypeVar(Provider) = {
	PyVarObject_HEAD_INIT(NULL, 0)
	/* tp_name */ 			"zeno.di.Provider",
	/* tp_basicsize */ 		sizeof(Provider),
	/* tp_itemsize */ 		0,
	/* tp_dealloc */ 		0,
	/* tp_print */ 			0,
	/* tp_getattr */ 		0,
	/* tp_setattr */ 		0,
	/* tp_as_async */ 		0,
	/* tp_repr */ 			0,
	/* tp_as_number */ 		0,
	/* tp_as_sequence */ 	0,
	/* tp_as_mapping */ 	0,
	/* tp_hash  */ 			0,
	/* tp_call */ 			0,
	/* tp_str */ 			0,
	/* tp_getattro */ 		0,
	/* tp_setattro */ 		0,
	/* tp_as_buffer */ 		0,
	/* tp_flags */ 			Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	/* tp_doc */ 			0,
	/* tp_traverse */ 		0,
	/* tp_clear */ 			0,
	/* tp_richcompare */ 	0,
	/* tp_weaklistoffset */ 0,
	/* tp_iter */ 			0,
	/* tp_iternext */ 		0,
	/* tp_methods */ 		0,
	/* tp_members */ 		0,
	/* tp_getset */ 		0,
	/* tp_base */ 			0,
	/* tp_dict */ 			0,
	/* tp_descr_get */ 		0,
	/* tp_descr_set */ 		0,
	/* tp_dictoffset */ 	0,
	/* tp_init */ 			0,
	/* tp_alloc */ 			0,
	/* tp_new */ 			Provider_new,
	/* tp_free */ 			0
};


ZenoDI_PrivateNew(ProviderExec)

ProviderExec* New(Provider* provider, Injector* injector) {
	assert(Provider::CheckExact(provider));
	assert(Injector::CheckExact(injector));

	ProviderExec* self = ProviderExec::Alloc();

	Py_INCREF(provider);
	Py_INCREF(injector);

	self->provider = provider;
	self->injector = injector;

	return self;
}

static PyObject* ProviderExec_call(ProviderExec* self, PyObject* args, PyObject** kwargs) {
	//TODO: ha vannak paraméterek, akkor hiba
	assert(Provider::CheckExact(self->provider));
	assert(Injector::CheckExact(self->injector));

	return Provider::Exec(self->provider, self->injector);
}

static void ProviderExec_dealloc(ProviderExec* self) {
	Py_XDECREF(self->provider);
	Py_XDECREF(self->injector);
	Py_TYPE(self)->tp_free((PyObject*) self);
}


PyTypeObject ZenoDI_TypeVar(ProviderExec) = {
	PyVarObject_HEAD_INIT(NULL, 0)
	/* tp_name */ 			"zeno.di.ProviderExec",
	/* tp_basicsize */ 		sizeof(ProviderExec),
	/* tp_itemsize */ 		0,
	/* tp_dealloc */ 		(destructor) ProviderExec_dealloc,
	/* tp_print */ 			0,
	/* tp_getattr */ 		0,
	/* tp_setattr */ 		0,
	/* tp_as_async */ 		0,
	/* tp_repr */ 			0,
	/* tp_as_number */ 		0,
	/* tp_as_sequence */ 	0,
	/* tp_as_mapping */ 	0,
	/* tp_hash  */ 			0,
	/* tp_call */ 			(ternaryfunc) ProviderExec_call,
	/* tp_str */ 			0,
	/* tp_getattro */ 		0,
	/* tp_setattro */ 		0,
	/* tp_as_buffer */ 		0,
	/* tp_flags */ 			Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	/* tp_doc */ 			0,
	/* tp_traverse */ 		0,
	/* tp_clear */ 			0,
	/* tp_richcompare */ 	0,
	/* tp_weaklistoffset */ 0,
	/* tp_iter */ 			0,
	/* tp_iternext */ 		0,
	/* tp_methods */ 		0,
	/* tp_members */ 		0,
	/* tp_getset */ 		0,
	/* tp_base */ 			0,
	/* tp_dict */ 			0,
	/* tp_descr_get */ 		0,
	/* tp_descr_set */ 		0,
	/* tp_dictoffset */ 	0,
	/* tp_init */ 			0,
	/* tp_alloc */ 			0,
	/* tp_new */ 			ProviderExec_new,
	/* tp_free */ 			0
};


} // end namespace ZenoDI

#endif /* AC1E491D_0133_C8FB_12AA_F65E6B95E8A0 */
