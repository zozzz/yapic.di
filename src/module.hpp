#ifndef AB77B960_F133_C8FB_1370_B184655AB464
#define AB77B960_F133_C8FB_1370_B184655AB464

#include <stdbool.h>
#include <Python.h>

#include "./errors.hpp"

// USE ONLY FOR DEBUGGING
#define ZenoDI_REPR(o) (o == NULL ? "<NULL>" : ((char*) PyUnicode_DATA(PyObject_Repr(o))))
#define ZenoDI_DUMP(o) printf(#o " = %s\n", ZenoDI_REPR(o))

namespace ZenoDI {

#define ZenoDI_MS_NewStr(target, str) \
	if ((st->target = PyUnicode_InternFromString(str)) == NULL) { \
		return false; \
	}

struct ModuleState {
	PyObject* str__code__;
	PyObject* str__defaults__;
	PyObject* str__kwdefaults__;
	PyObject* str__annotations__;
	PyObject* str__qualname__;
	PyObject* str__call__;
	PyObject* str__init__;
	PyObject* str__self__;

	PyObject* ExcBase;
	PyObject* ExcProvideError;
	PyObject* MethodWrapperType;

	static inline bool Init(PyObject* module) {
		ModuleState* st = Get(module);
		ZenoDI_MS_NewStr(str__code__, "__code__")
		ZenoDI_MS_NewStr(str__defaults__, "__defaults__")
		ZenoDI_MS_NewStr(str__kwdefaults__, "__kwdefaults__")
		ZenoDI_MS_NewStr(str__annotations__, "__annotations__")
		ZenoDI_MS_NewStr(str__qualname__, "__qualname__")
		ZenoDI_MS_NewStr(str__call__, "__call__")
		ZenoDI_MS_NewStr(str__init__, "__init__")
		ZenoDI_MS_NewStr(str__self__, "__self__")

		if ((st->ExcBase = PyErr_NewException("zeno.di.InjectorError", NULL, NULL)) == NULL) {
			return false;
		} else {
			PyModule_AddObject(module, "InjectorError", st->ExcBase);
		}

		if ((st->ExcProvideError = PyErr_NewException("zeno.di.ProvideError", st->ExcBase, NULL)) == NULL) {
			return false;
		} else {
			PyModule_AddObject(module, "ProvideError", st->ExcProvideError);
		}

		PyObject* method = PyObject_GetAttrString(st->ExcBase, "__call__");
		if (method == NULL) {
			return false;
		}
		st->MethodWrapperType = (PyObject*) Py_TYPE(method);
		Py_DECREF(method);

		return true;
	}

	static int Clear(PyObject* module) {
		ModuleState* st = Get(module);
		Py_CLEAR(st->str__code__);
		Py_CLEAR(st->str__defaults__);
		Py_CLEAR(st->str__kwdefaults__);
		Py_CLEAR(st->str__annotations__);
		Py_CLEAR(st->str__qualname__);
		Py_CLEAR(st->str__call__);
		Py_CLEAR(st->str__init__);
		Py_CLEAR(st->str__self__);
		Py_CLEAR(st->ExcBase);
		Py_CLEAR(st->ExcProvideError);
		return 0;
	}

	static inline ModuleState* Get();
	static inline ModuleState* Get(PyObject* module);
};


#define ZenoDI_TypeVar(name) ZenoDI :: name :: Type

#define ZenoDI_CheckFn(name) \
	static inline bool CheckExact(PyObject* obj) { \
		return obj != NULL && Py_TYPE(obj) == &ZenoDI_TypeVar(name); \
	} \
	static inline bool Check(PyObject* obj) { \
		return obj != NULL && PyObject_TypeCheck(obj, &ZenoDI_TypeVar(name)); \
	} \
	static inline bool CheckExact(name* obj) { return CheckExact((PyObject*) obj); } \
	static inline bool Check(name* obj) { return Check((PyObject*) obj); }

#define ZenoDI_AllocFn(name) \
	static inline name* Alloc() { \
		return (name*) ZenoDI_TypeVar(name).tp_alloc(&ZenoDI_TypeVar(name), 0); \
	}

#define ZenoDI_Type(name) \
	static PyTypeObject Type; \
	ZenoDI_AllocFn(name) \
	ZenoDI_CheckFn(name)

#define ZenoDI_AddType(name) \
	if (PyType_Ready(&ZenoDI_TypeVar(name)) < 0) { return NULL; } \
	PyModule_AddObject(module, #name, (PyObject*) &ZenoDI_TypeVar(name))

#define ZenoDI_PrivateNew(name) \
	static PyObject* name ## _new(PyTypeObject *type, PyObject *args, PyObject *kwargs) { \
		return NULL; \
	}




struct Injector {
	PyObject_HEAD
	PyObject* scope;
	Injector* parent;

	static Injector* New(Injector* parent, PyObject* scope);
	static PyObject* Find(Injector* injector, PyObject* id);
	// static PyObject* Descend(Injector* injector); // ???

	ZenoDI_Type(Injector)
};


struct Provider {
	enum ValueType {
		CLASS = 1,
		FUNCTION = 2,
		OTHER = 4
	};

	enum Strategy {
		FACTORY = 1,
		VALUE = 2,
		GLOBAL = 4,
		SINGLETON = 8,
		CUSTOM = 16,
		ALL = FACTORY | VALUE | GLOBAL | SINGLETON | CUSTOM
	};

	PyObject_HEAD
	PyObject* value;
	PyObject* args; 		// tuple[ValueResolver]
	PyObject* kwargs;		// dict[str, ValueResolver]
	PyObject* attributes;	// dict[str, ValueResolver]
	PyObject* own_scope;
	PyObject* custom_strategy;

	ValueType value_type;
	Strategy strategy;

	static Provider* New(PyObject* value, Strategy strategy, PyObject* provide);
	static Provider* New(PyObject* value, PyObject* strategy, PyObject* provide);
	static PyObject* Resolve(Provider* self, Injector* injector);
	static PyObject* Exec(Provider* self, Injector* injector);

	ZenoDI_Type(Provider)
};


struct ProviderExec {
	PyObject_HEAD
	Provider* provider;
	Injector* injector;

	static ProviderExec* New(Provider* provider, Injector* injector);

	ZenoDI_Type(ProviderExec)
};


// struct NameResolver {
// 	enum Mode {
// 		DICT = 1,
// 		CALLABLE = 2
// 	};

// 	PyObject_HEAD
// 	PyObject* from;
// 	Mode mode;

// 	static PyObject* New(PyObject* from, Mode mode);
// 	static PyObject* Resolve(NameResolver* self, Injector* injector, PyObject* name);
// };


struct ValueResolver {
	PyObject_HEAD
	PyObject* id;
	PyObject* name;
	PyObject* default_value;
	// NameResolver* name_resolver;

	static ValueResolver* New(PyObject* name, PyObject* id, PyObject* default_value);
	static PyObject* Resolve(ValueResolver* self, Injector* injector);

	ZenoDI_Type(ValueResolver)
};


// struct AttributeResolver {
// 	PyObject_HEAD
//	Injector* injector;
// 	NameResolver* resolver;

// 	static AttributeResolver* New(Injector* injector, NameResolver* resolver);

// 	ZenoDI_Type(AttributeResolver)
// };


// FOR: USE ENUM AS FLAG
template<class T> inline T operator~ (T a) { return (T)~(int)a; }
template<class T> inline T operator| (T a, T b) { return (T)((int)a | (int)b); }
template<class T> inline T operator& (T a, T b) { return (T)((int)a & (int)b); }
template<class T> inline T operator^ (T a, T b) { return (T)((int)a ^ (int)b); }
template<class T> inline T& operator|= (T& a, T b) { return (T&)((int&)a |= (int)b); }
template<class T> inline T& operator&= (T& a, T b) { return (T&)((int&)a &= (int)b); }
template<class T> inline T& operator^= (T& a, T b) { return (T&)((int&)a ^= (int)b); }

} // end namespace ZenoDI

#endif /* AB77B960_F133_C8FB_1370_B184655AB464 */
