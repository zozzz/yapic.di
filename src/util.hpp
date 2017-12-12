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


/**
 * example:
 *  PyObject* res = ResolveNameFromGlobals(fn.__globals__, "imported_module.ClassInModule")
 * 	res == ClassInModule
 *
 * returns New reference
 */
static inline PyObject* ResolveNameFromGlobals(PyObject* globals, const char* name, Py_ssize_t len) {
	Py_INCREF(globals);
	PyPtr<> res = globals;

	char* cursor = const_cast<char*>(name);
	const char* end = name + len;
	char* dot = NULL;
	do {
		dot = strchr(cursor, '.');
		if (dot == NULL) {
			dot = const_cast<char*>(name) + len;
		}

		PyPtr<> key = PyUnicode_FromStringAndSize(cursor, dot - cursor);
		if (key.IsNull()) {
			return NULL;
		}
		if (PyDict_CheckExact(res)) {
			res = PyDict_GetItemWithError(res, key);
			if (res.IsNull()) {
				return NULL;
			}
			res.Incref();
		} else {
			res = PyObject_GetAttr(res, key);
			if (res.IsNull()) {
				return NULL;
			}
		}
		cursor = dot + 1;
	} while (dot < end);

	return res.Steal();
}


static inline PyObject* ResolveNameFromGlobals(PyObject* globals, PyObject* name) {
	assert(name && PyUnicode_CheckExact(name));
	return ResolveNameFromGlobals(globals, (const char*) PyUnicode_1BYTE_DATA(name), PyUnicode_GET_LENGTH(name));
}


// static inline PyObject* ImportModule(const char* name, Py_ssize_t len) {
// 	char* lastDot = strrchr(name, '.');
// 	PyPtr<> fromlist;
// 	PyPtr<> moduleName;

// 	if (lastDot != NULL) {
// 		fromlist = PyList_New(1);
// 		if (fromlist == NULL) {
// 			return NULL;
// 		}

// 		PyObject* lastPart = PyUnicode_FromStringAndSize(lastDot, name - lastDot);
// 		if (lastPart == NULL) {
// 			return NULL;
// 		}
// 		PyList_SET_ITEM(fromlist, 0, lastPart);
// 	} else {
// 		moduleName = PyUnicode_FromStringAndSize
// 	}
// 	PyObject* module = PyImport_ImportModuleEx(name, NULL, NULL)
// }


static inline PyObject* ImportModule(PyObject* name) {
	assert(PyUnicode_CheckExact(name));

	Py_ssize_t len = PyUnicode_GET_LENGTH(name);
	Py_ssize_t lastDot = PyUnicode_FindChar(name, '.', 0, len, -1);

	if (lastDot == -2) {
		return NULL;
	} else if (lastDot > 0) {
		PyPtr<> fromlist = PyList_New(1);
		if (fromlist.IsNull()) {
			return NULL;
		}

		PyObject* lastPart = PyUnicode_Substring(name, lastDot, len);
		if (lastPart == NULL) {
			return NULL;
		}
		PyList_SET_ITEM(fromlist.As<PyListObject>(), 0, lastPart);

		PyPtr<> mName = PyUnicode_Substring(name, 0, lastDot - 1);
		if (mName.IsNull()) {
			return NULL;
		}

		return PyImport_ImportModuleLevelObject(mName, NULL, NULL, fromlist, 0);
	} else {
		return PyImport_ImportModuleLevelObject(name, NULL, NULL, NULL, 0);
	}
}

} /* end namespace ZenoDI */

#endif /* A1C878A6_5133_C9C5_139C_3B338F5A63F6 */
