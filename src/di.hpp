#ifndef AB77B960_F133_C8FB_1370_B184655AB464
#define AB77B960_F133_C8FB_1370_B184655AB464

// USE ONLY FOR DEBUGGING
#define ZenoDI_REPR(o) (((PyObject*)o) == NULL ? "<NULL>" : ((char*) PyUnicode_DATA(PyObject_Repr(((PyObject*)o)))))
#define ZenoDI_DUMP(o) printf(#o " = %s\n", ZenoDI_REPR(o))

#include <stdbool.h>
#include <Python.h>
#include <yapic/module.hpp>
#include <yapic/type.hpp>
#include <yapic/pyptr.hpp>
#include <yapic/string-builder.hpp>

#include "./errors.hpp"

namespace ZenoDI {

using Yapic::PyPtr;

class Injector: public Yapic::Type<Injector, Yapic::Object> {
public:
	PyObject* scope;
	PyObject* kwargs;
	Injector* parent;

	static Injector* New(Injector* parent);
	static Injector* New(Injector* parent, PyObject* scope);
	static PyObject* Find(Injector* injector, PyObject* id);
	static PyObject* Provide(Injector* injector, PyObject* id);
	static PyObject* Provide(Injector* injector, PyObject* id, PyObject* value, PyObject* strategy, PyObject* provide);
	static void SetParent(Injector* injector, Injector* parent);
	static Injector* Clone(Injector* injector);
	static Injector* Clone(Injector* injector, Injector* parent);

	static PyObject* __new__(PyTypeObject *type, PyObject *args, PyObject *kwargs);
	static void __dealloc__(Injector* self);
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
	using UnicodeBuilder = Yapic::UnicodeBuilder<1024>;

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

	PyObject* value;
	PyObject* args; 		// tuple[ValueResolver]
	PyObject* kwargs;		// dict[str, ValueResolver]
	PyObject* attributes;	// dict[str, ValueResolver]
	Injector* own_injector;
	PyObject* custom_strategy;

	ValueType value_type;
	Strategy strategy;

	Yapic_PrivateNew;

	static Injectable* New(PyObject* value, Strategy strategy, PyObject* provide);
	static Injectable* New(PyObject* value, PyObject* strategy, PyObject* provide);
	static PyObject* Resolve(Injectable* self, Injector* injector);
	static bool ToString(Injectable* self, UnicodeBuilder* builder, int level);
	// static PyObject* Exec(Injectable* self, Injector* injector);
	// egy injectort vár paraméternek, és így vissza tudja adni azt, amit kell
	static PyObject* __call__(Injectable* self, PyObject* args, PyObject** kwargs);
	static PyObject* __repr__(Injectable* self);
	static void __dealloc__(Injectable* self);
	// visszaad egy bounded injectable objektumot, aminek már nem kell megadni
	// az injector objektumot, akkor ha meghívjuk
	static PyObject* bind(Injectable* self, Injector* injector);

	Yapic_METHODS_BEGIN
		Yapic_Method(bind, METH_O, "")
	Yapic_METHODS_END
};


class BoundInjectable: public Yapic::Type<BoundInjectable, Yapic::Object> {
public:
	Injectable* injectable;
	Injector* injector;

	Yapic_PrivateNew;

	static BoundInjectable* New(Injectable* injectable, Injector* injector);
	static PyObject* __call__(BoundInjectable* self, PyObject* args, PyObject** kwargs);
	static void __dealloc__(BoundInjectable* self);
};


class ValueResolver: public Yapic::Type<ValueResolver, Yapic::Object> {
public:
	PyObject* id;
	PyObject* name;
	PyObject* default_value;

	Yapic_PrivateNew;

	static ValueResolver* New(PyObject* name, PyObject* id, PyObject* default_value);
	template<bool UseKwOnly>
	static PyObject* Resolve(ValueResolver* self, Injector* injector, Injector* own_injector);
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
	static PyObject* Resolve(KwOnly* self, Injector* injector, PyObject* name, PyObject* type);
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
	// ModuleVar STR_ORIGIN;

	ModuleVar FACTORY;
	ModuleVar VALUE;
	ModuleVar GLOBAL;
	ModuleVar SINGLETON;

	ModuleExc ExcBase;
	ModuleExc ExcProvideError;
	ModuleExc ExcInjectError;
	ModuleExc ExcNoKwOnly;

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
		// state->STR_ORIGIN = "__origin__";

		state->VALUE.Value(Injectable::Strategy::VALUE).Export("VALUE");
		state->FACTORY.Value(Injectable::Strategy::FACTORY).Export("FACTORY");
		state->SINGLETON.Value(
			Injectable::Strategy::SINGLETON |
			Injectable::Strategy::FACTORY
		).Export("SINGLETON");
		state->GLOBAL.Value(
			Injectable::Strategy::GLOBAL |
			Injectable::Strategy::SINGLETON |
			Injectable::Strategy::FACTORY
		).Export("GLOBAL");

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

		Injector::Register(module);
		Injectable::Register(module);
		BoundInjectable::Register(module);
		ValueResolver::Register(module);
		KwOnly::Register(module);

		return 0;
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
