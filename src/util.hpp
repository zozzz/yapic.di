#ifndef A1C878A6_5133_C9C5_139C_3B338F5A63F6
#define A1C878A6_5133_C9C5_139C_3B338F5A63F6

#include "./di.hpp"

namespace ZenoDI {

/**
 * example:
 * 	class A(Generic[T]):
 * 		pass
 *
 *  s = ResolveTypeVars(A[Something])
 * 	s = {~T: Something}
 *
 * returns NULL if given type is not Generic (not raise error) or error raised
 */
static inline PyObject* ResolveTypeVars(PyObject* type) {
	PyObject* mro = ((PyTypeObject*) type)->tp_mro;
	assert(mro != NULL);
	assert(PyTuple_CheckExact(mro));
	Py_ssize_t mroSize = PyTuple_GET_SIZE(mro);

	if (mroSize < 2) {
		return NULL;
	}

	PyObject* __params = Module::State()->STR_PARAMETERS;
	PyPtr<> params = PyObject_GetAttr(PyTuple_GET_ITEM(mro, 1), __params);
	if (params.IsNull()) {
		PyErr_Clear();
		return NULL;
	}

	Py_ssize_t pSize = PyTuple_GET_SIZE(params.As<PyTupleObject>());
	if (pSize == 0) {
		return NULL;
	}

	PyPtr<> args = PyObject_GetAttr(type, Module::State()->STR_ARGS);
	if (args.IsNull() || !PyTuple_CheckExact(args)) {
		// TODO: another error...
		PyErr_Format(Module::State()->ExcProvideError, ZenoDI_Err_IllegalGenericProvide, type);
		return NULL;
	}

	Py_ssize_t aSize = PyTuple_GET_SIZE(args.As<PyTupleObject>());
	if (aSize != pSize) {
		PyErr_Format(Module::State()->ExcProvideError, ZenoDI_Err_IllegalGenericProvide, type);
		return NULL;
	}

	PyPtr<> vars = PyDict_New();
	if (vars.IsNull()) {
		return NULL;
	}

	for (Py_ssize_t i=0 ; i<pSize ; ++i) {
		if (PyDict_SetItem(vars, PyTuple_GET_ITEM(params, i), PyTuple_GET_ITEM(args, i)) < 0) {
			return NULL;
		}
	}

	for (Py_ssize_t i=2 ; i<mroSize ; ++i) {
		PyObject* cls = PyTuple_GET_ITEM(mro, i);
		params = PyObject_GetAttr(cls, __params);
		if (params.IsNull()) {
			PyErr_Clear();
			continue;
		} else if (PyTuple_CheckExact(params)) {
			pSize = PyTuple_GET_SIZE(params);
			Py_ssize_t l = pSize > aSize ? aSize : pSize;
			for (Py_ssize_t k=0 ; k<l ; ++k) {
				if (PyDict_SetItem(vars, PyTuple_GET_ITEM(params, k), PyTuple_GET_ITEM(args, k)) < 0) {
					return NULL;
				}
			}
		}
	}

	return vars.Steal();
}

} /* end namespace ZenoDI */

#endif /* A1C878A6_5133_C9C5_139C_3B338F5A63F6 */
