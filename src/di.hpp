#ifndef AB77B960_F133_C8FB_1370_B184655AB464
#define AB77B960_F133_C8FB_1370_B184655AB464

#include <stdbool.h>
#include <Python.h>
#include <yapic/module.hpp>
#include <yapic/type.hpp>

#include "./errors.hpp"

// USE ONLY FOR DEBUGGING
#define ZenoDI_REPR(o) (o == NULL ? "<NULL>" : ((char*) PyUnicode_DATA(PyObject_Repr(o))))
#define ZenoDI_DUMP(o) printf(#o " = %s\n", ZenoDI_REPR(o))

namespace ZenoDI {

class Injector: public Yapic::Type<Injector, Yapic::Object> {
public:
	PyObject* scope;
	Injector* parent;

	static Injector* New(Injector* parent, PyObject* scope);
	static PyObject* Find(Injector* injector, PyObject* id);

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


class Provider: public Yapic::Type<Provider, Yapic::Object> {
public:
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
	PyObject* own_scope;
	PyObject* custom_strategy;

	ValueType value_type;
	Strategy strategy;

	Yapic_PrivateNew;

	static Provider* New(PyObject* value, Strategy strategy, PyObject* provide);
	static Provider* New(PyObject* value, PyObject* strategy, PyObject* provide);
	static PyObject* Resolve(Provider* self, Injector* injector);
	// static PyObject* Exec(Provider* self, Injector* injector);
	// egy injectort vár paraméternek, és így vissza tudja adni azt, amit kell
	static PyObject* __call__(Provider* self, PyObject* args, PyObject** kwargs);
	// visszaad egy bounded provider objektumot, aminek már nem kell megadni
	// az injector objektumot, akkor ha meghívjuk
	static PyObject* bind(Provider* self, Injector* injector);

	Yapic_METHODS_BEGIN
		Yapic_Method(bind, METH_O, "");
	Yapic_METHODS_END
};


class BoundProvider: public Yapic::Type<BoundProvider, Yapic::Object> {
public:
	Provider* provider;
	Injector* injector;

	Yapic_PrivateNew;

	static BoundProvider* New(Provider* provider, Injector* injector);
	static PyObject* __call__(BoundProvider* self, PyObject* args, PyObject** kwargs);
	static void __dealloc__(BoundProvider* self);
};


class ValueResolver: public Yapic::Type<ValueResolver, Yapic::Object> {
public:
	PyObject* id;
	PyObject* name;
	PyObject* default_value;
	// NameResolver* name_resolver;

	Yapic_PrivateNew;

	static ValueResolver* New(PyObject* name, PyObject* id, PyObject* default_value);
	static PyObject* ResolveArgument(ValueResolver* self, Injector* injector);
	static PyObject* ResolveAttribute(ValueResolver* self, Injector* injector);
	static void __dealloc__(ValueResolver* self);
	static PyObject* __repr__(ValueResolver* self);
};


class Module: public Yapic::Module<Module> {
public:
	using ModuleVar = Yapic::ModuleVar<Module>;
	using ModuleExc = Yapic::ModuleExc<Module>;
	using ModuleRef = Yapic::ModuleRef<Module>;

	static constexpr const char* __name__ = "zeno.di";

	ModuleVar STR_CODE;
	ModuleVar STR_DEFAULTS;
	ModuleVar STR_KWDEFAULTS;
	ModuleVar STR_ANNOTATIONS;
	ModuleVar STR_QUALNAME;
	ModuleVar STR_CALL;
	ModuleVar STR_INIT;
	ModuleVar STR_SELF;

	ModuleExc ExcBase;
	ModuleExc ExcProvideError;

	PyObject* MethodWrapperType;

	static inline int __init__(PyObject* module, Module* state) {
		state->STR_CODE = "__code__";
		state->STR_DEFAULTS = "__defaults__";
		state->STR_KWDEFAULTS = "__kwdefaults__";
		state->STR_ANNOTATIONS = "__annotations__";
		state->STR_QUALNAME = "__qualname__";
		state->STR_CALL = "__call__";
		state->STR_INIT = "__init__";
		state->STR_SELF = "__self__";

		state->ExcBase.Define("InjectorError");
		state->ExcProvideError.Define("ProvideError", state->ExcBase);

		// init MethodWrapperType
		PyObject* method = PyObject_GetAttrString(state->ExcBase, "__call__");
		if (method == NULL) {
			return false;
		}
		state->MethodWrapperType = (PyObject*) Py_TYPE(method);
		Py_DECREF(method);

		Injector::Register(module);
		Provider::Register(module);
		BoundProvider::Register(module);
		ValueResolver::Register(module);

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
