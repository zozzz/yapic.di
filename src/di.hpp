#ifndef AB77B960_F133_C8FB_1370_B184655AB464
#define AB77B960_F133_C8FB_1370_B184655AB464

// USE ONLY FOR DEBUGGING
#define ZenoDI_REPR(o) (((PyObject*)o) == NULL ? "<NULL>" : ((char*) PyUnicode_DATA(PyObject_Repr(((PyObject*)o)))))
#define ZenoDI_DUMP(o) printf(#o " = %s\n", ZenoDI_REPR(o))

#define ZenoDI_MAX_RECURSION 1000

#ifdef _MSC_VER
#	define FORCEINLINE __forceinline
#elif defined(__GNUC__)
#	define FORCEINLINE __attribute__((always_inline)) inline
#else
#	define FORCEINLINE inline
#endif

#include <stdbool.h>
#include <Python.h>
#include <yapic/module.hpp>
#include <yapic/type.hpp>
#include <yapic/pyptr.hpp>
#include <yapic/string-builder.hpp>
#include <yapic/thread.hpp>

#include "./errors.hpp"

namespace ZenoDI {

using Yapic::PyPtr;

class Injector: public Yapic::Type<Injector, Yapic::Object> {
public:
	PyObject* injectables;
	PyObject* singletons;
	PyObject* kwargs;
	Injector* parent;

	static Injector* New(Injector* parent);
	static inline PyObject* Find(Injector* injector, PyObject* id);
	static inline PyObject* Find(Injector* injector, PyObject* id, Py_hash_t hash);
	static PyObject* Provide(Injector* injector, PyObject* id);
	static PyObject* Provide(Injector* injector, PyObject* id, PyObject* value, PyObject* strategy, PyObject* provide);
	static void SetParent(Injector* injector, Injector* parent);
	static Injector* Clone(Injector* injector);
	static Injector* Clone(Injector* injector, Injector* parent);

	static PyObject* __new__(PyTypeObject *type, PyObject *args, PyObject *kwargs);
	static void __dealloc__(Injector* self);
	static PyObject* __mp_getitem__(Injector* self, PyObject* key);
	static PyObject* provide(Injector* self, PyObject* args, PyObject* kwargs);
	static PyObject* get(Injector* self, PyObject* id);
	static PyObject* exec(Injector* self, PyObject* args, PyObject* kwargs);
	static PyObject* injectable(Injector* self, PyObject* args, PyObject* kwargs);
	static PyObject* descend(Injector* self);

	Yapic_METHODS_BEGIN
		Yapic_Method(provide, METH_VARARGS | METH_KEYWORDS, "")
		Yapic_Method(get, METH_O, "")
		Yapic_Method(exec, METH_VARARGS | METH_KEYWORDS, "")
		Yapic_Method(injectable, METH_VARARGS | METH_KEYWORDS, "")
		Yapic_Method(descend, METH_NOARGS, "")
	Yapic_METHODS_END
};


class Injectable: public Yapic::Type<Injectable, Yapic::Object> {
public:
	typedef PyObject* (*StrategyCallback)(Injectable* self, Injector* injector, Injector* owni, int recursion);
	using UnicodeBuilder = Yapic::UnicodeBuilder<1024>;

	enum ValueType {
		CLASS = 1,
		FUNCTION = 2,
		OTHER = 4
	};

	enum Strategy {
		FACTORY = 1,
		SINGLETON = 2,
		SCOPED = 3,
		CUSTOM = 4,
		VALUE = 5,
		MAX = 5
		// ALL = FACTORY | VALUE | SINGLETON | SCOPED | CUSTOM
	};

	PyObject* value;
	PyObject* args; 		// tuple[ValueResolver]
	PyObject* kwargs;		// dict[str, ValueResolver]
	PyObject* attributes;	// dict[str, ValueResolver]
	Injector* own_injector;
	// PyObject* custom_strategy;
	PyObject* resolved;		// in singleton strategy this is the resolved value, if CUSTOM, this is a callable

	Py_hash_t hash;
	// ValueType value_type;
	// Strategy strategy;
	StrategyCallback strategy;
	StrategyCallback get_value;

	Yapic_PrivateNew;

	static Injectable* New(PyObject* value, Strategy strategy, PyObject* provide);
	static Injectable* New(PyObject* value, PyObject* strategy, PyObject* provide);
	static inline PyObject* Resolve(Injectable* self, Injector* injector, int recursion);
	static bool ToString(Injectable* self, UnicodeBuilder* builder, int level);
	// static PyObject* Exec(Injectable* self, Injector* injector);
	// egy injectort vár paraméternek, és így vissza tudja adni azt, amit kell
	static PyObject* __call__(Injectable* self, PyObject* args, PyObject** kwargs);
	static Py_hash_t __hash__(Injectable* self);
	static PyObject* __cmp__(Injectable* self, PyObject* other, int op);
	static PyObject* __repr__(Injectable* self);
	static void __dealloc__(Injectable* self);
	// visszaad egy bounded injectable objektumot, aminek már nem kell megadni
	// az injector objektumot, akkor ha meghívjuk
	static PyObject* bind(Injectable* self, Injector* injector);
	static PyObject* resolve(Injectable* self, Injector* injector);

	Yapic_METHODS_BEGIN
		Yapic_Method(bind, METH_O, "")
		Yapic_Method(resolve, METH_O, "")
	Yapic_METHODS_END
};


class BoundInjectable: public Yapic::Type<BoundInjectable, Yapic::Object> {
public:
	Injectable* injectable;
	Injector* injector;
	Py_hash_t hash;

	Yapic_PrivateNew;

	static BoundInjectable* New(Injectable* injectable, Injector* injector, Py_hash_t hash);
	static PyObject* __call__(BoundInjectable* self, PyObject* args, PyObject** kwargs);
	static Py_hash_t __hash__(BoundInjectable* self);
	static PyObject* __cmp__(BoundInjectable* self, PyObject* other, int op);
	static void __dealloc__(BoundInjectable* self);
};


// class InjectableFactory: public Yapic::Type<InjectableFactory, Yapic::Object, Yapic::FreeList<InjectableFactory, 256>> {
// public:
// 	Injectable* injectable;
// 	Injector* injector;
// 	Py_hash_t hash;

// 	Yapic_PrivateNew;

// 	static InjectableFactory* New(Injectable* injectable, Injector* injector, Py_hash_t hash);
// 	static PyObject* __call__(InjectableFactory* self, PyObject* args, PyObject** kwargs);
// 	static Py_hash_t __hash__(InjectableFactory* self);
// 	static PyObject* __cmp__(InjectableFactory* self, PyObject* other, int op);
// 	static void __dealloc__(InjectableFactory* self);
// };


class ValueResolver: public Yapic::Type<ValueResolver, Yapic::Object> {
public:
	PyObject* id;
	PyObject* name;
	PyObject* default_value;
	PyObject* globals;
	Py_hash_t id_hash;

	Yapic_PrivateNew;

	static ValueResolver* New(PyObject* name, PyObject* id, PyObject* default_value, PyObject* globals);
	template<bool UseKwOnly>
	static PyObject* Resolve(ValueResolver* self, Injector* injector, Injector* own_injector, int recursion);
	static void SetId(ValueResolver* self, PyObject* id);
	static void SetName(ValueResolver* self, PyObject* name);
	static void SetDefaultValue(ValueResolver* self, PyObject* value);
	static void __dealloc__(ValueResolver* self);
	static PyObject* __repr__(ValueResolver* self);
};


class KwOnly: public Yapic::Type<KwOnly, Yapic::Object> {
public:
	Injectable* getter;
	ValueResolver* name_resolver;
	ValueResolver* type_resolver;

	static KwOnly* New(PyObject* getter);
	static PyObject* Resolve(KwOnly* self, Injector* injector, PyObject* name, PyObject* type, int recursion);
	static PyObject* __new__(PyTypeObject *type, PyObject *args, PyObject *kwargs);
	static void __dealloc__(KwOnly* self);
	static PyObject* __repr__(KwOnly* self);
};


class Module: public Yapic::Module<Module> {
public:
	using ModuleVar = Yapic::ModuleVar<Module>;
	using ModuleExc = Yapic::ModuleExc<Module>;
	using ModuleRef = Yapic::ModuleRef<Module>;

	static constexpr const char* __name__ = "zeno.di";

	ModuleVar STR_ANNOTATIONS;
	ModuleVar STR_QUALNAME;
	ModuleVar STR_CALL;
	ModuleVar STR_INIT;
	ModuleVar STR_KWA_NAME;
	ModuleVar STR_KWA_TYPE;
	ModuleVar STR_ARGS;
	ModuleVar STR_PARAMETERS;
	ModuleVar STR_MODULE;
	ModuleVar singletons;

	ModuleVar FACTORY;
	ModuleVar VALUE;
	ModuleVar GLOBAL;
	ModuleVar SINGLETON;

	ModuleExc ExcBase;
	ModuleExc ExcProvideError;
	ModuleExc ExcInjectError;
	ModuleExc ExcNoKwOnly;

	Yapic::RLock* rlock_singletons;

	PyObject* MethodWrapperType;

	static inline int __init__(PyObject* module, Module* state) {
		state->STR_ANNOTATIONS = "__annotations__";
		state->STR_QUALNAME = "__qualname__";
		state->STR_CALL = "__call__";
		state->STR_INIT = "__init__";
		state->STR_KWA_NAME = "name";
		state->STR_KWA_TYPE = "type";
		state->STR_ARGS = "__args__";
		state->STR_PARAMETERS = "__parameters__";
		state->STR_MODULE = "__module__";
		state->singletons = PyDict_New();

		state->VALUE.Value(Injectable::Strategy::VALUE).Export("VALUE");
		state->FACTORY.Value(Injectable::Strategy::FACTORY).Export("FACTORY");
		state->SINGLETON.Value(Injectable::Strategy::SCOPED).Export("SCOPED_SINGLETON");
		state->GLOBAL.Value(Injectable::Strategy::SINGLETON).Export("SINGLETON");

		state->ExcBase.Define("InjectorError", PyExc_TypeError);
		state->ExcProvideError.Define("ProvideError", state->ExcBase);
		state->ExcInjectError.Define("InjectError", state->ExcBase);
		state->ExcNoKwOnly.Define("NoKwOnly", state->ExcProvideError);

		// init MethodWrapperType
		PyObject* method = PyObject_GetAttrString(state->ExcBase, "__call__");
		if (method == NULL) {
			return false;
		}
		state->MethodWrapperType = (PyObject*) Py_TYPE(method);
		Py_DECREF(method);

		if (!Injector::Register(module) ||
			!Injectable::Register(module) ||
			!BoundInjectable::Register(module) ||
			// !InjectableFactory::Register(module) ||
			!ValueResolver::Register(module) ||
			!KwOnly::Register(module)) {
			return -1;
		}

		state->rlock_singletons = new Yapic::RLock();
		if (state->rlock_singletons->IsNull()) {
			return -1;
		}

		return 0;
	}

	static inline int __clear__(PyObject* module) {
		Self* state = State(module);
		delete state->rlock_singletons;
		return Super::__clear__(module);
	}
};


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
