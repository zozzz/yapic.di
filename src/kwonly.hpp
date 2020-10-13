#ifndef C7CADC2B_1133_C962_F786_A62FCBE6BEC8
#define C7CADC2B_1133_C962_F786_A62FCBE6BEC8

#include "./di.hpp"

namespace YapicDI {

KwOnly* KwOnly::New(PyObject* getter) {
	PyPtr<KwOnly> self = KwOnly::Alloc();
	if (self.IsNull()) {
		return NULL;
	}

	if (PyType_Check(getter) || !PyCallable_Check(getter)) {
		PyErr_SetString(Module::State()->ExcProvideError, YapicDI_Err_CallableArgument);
		return NULL;
	}

	self->getter = Injectable::New(getter, Injectable::Strategy::FACTORY, NULL);
	if (self->getter == NULL) {
		return NULL;
	}

	if (!self->getter->kwargs) {
		PyErr_SetString(Module::State()->ExcProvideError, YapicDI_Err_CallableMustHaveKwOnly);
		return NULL;
	}

	self->name_resolver = Injectable::GetKwArg(self->getter, Module::State()->STR_KWA_NAME);
	if (self->name_resolver == NULL) {
		PyErr_SetString(Module::State()->ExcProvideError, YapicDI_Err_KwOnlyGetterMustHaveNameKw);
		return NULL;
	} else {
		assert(ValueResolver::CheckExact(self->name_resolver));
		Py_INCREF(self->name_resolver);
	}

	self->type_resolver = Injectable::GetKwArg(self->getter, Module::State()->STR_KWA_TYPE);
	if (self->type_resolver != NULL) {
		assert(ValueResolver::CheckExact(self->type_resolver));
		Py_INCREF(self->type_resolver);
	}

	return self.Steal();
}


PyObject* KwOnly::Resolve(KwOnly* self, Injector* injector, PyObject* name, PyObject* type, int recursion) {
	assert(Injector::CheckExact(injector));

	ValueResolver::SetDefaultValue(self->name_resolver, name);
	if (self->type_resolver && type != NULL) {
		ValueResolver::SetDefaultValue(self->type_resolver, type);
	}

	assert(PyCallable_Check(self->getter->value));
	// PyObject* value = Injectable::Resolve(self->getter, injector, recursion);
	PyObject* value = _injectable::KwOnlyGetter::Get(self->getter, injector, NULL, recursion);
	if (value == NULL) {
		PyObject* exc = PyErr_Occurred(); // borrowed
		if (PyErr_GivenExceptionMatches(exc, Module::State()->ExcNoKwOnly)) {
			PyErr_Clear();
			return NULL;
		} else {
			return NULL;
		}
	} else {
		return value;
	}
}


PyObject* KwOnly::__new__(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
	static char *kwlist[] = {"get", NULL};

	PyObject* getter;
	if (PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &getter)) {
		if (!PyCallable_Check(getter)) {
			PyErr_SetString(PyExc_TypeError, YapicDI_Err_CallableArgument);
			return NULL;
		}
		return (PyObject*) KwOnly::New(getter);
	}

	return NULL;
}


void KwOnly::__dealloc__(KwOnly* self) {
	Py_CLEAR(self->getter);
	Py_CLEAR(self->name_resolver);
	Py_CLEAR(self->type_resolver);
	Py_TYPE(self)->tp_free((PyObject*)self);
}


PyObject* KwOnly::__repr__(KwOnly* self) {
	return PyUnicode_FromFormat("<KwOnly %R>", self->getter);
}


} /* end namespace YapicDI */

#endif /* C7CADC2B_1133_C962_F786_A62FCBE6BEC8 */
