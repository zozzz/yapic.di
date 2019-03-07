#ifndef CA8FD8DC_E134_145E_1469_7F5D029ABE37
#define CA8FD8DC_E134_145E_1469_7F5D029ABE37

#include "./di.hpp"
#include "./util.hpp"


namespace ZenoDI { namespace Typing {


static inline bool IsGeneric(const PyTypeObject* value) {
    return Module::State()->Generic.IsSubclass(value);
}


static inline bool IsGenericType(const PyObject* value) {
    return Module::State()->GenericAlias.Check(value);
}


static inline PyObject* GenericTypeOrigin(const PyObject* value) {
    return PyObject_GetAttr(const_cast<PyObject*>(value), Module::State()->STR_ORIGIN);
}


static inline bool IsInjectableAttr(PyObject* value) {
    if (IsGenericType(value)) {
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


class MroResolver {
    public:
        MroResolver(const PyObject* type, const PyObject* typeVars)
            : _type(type), _typeVars(typeVars), _mro(NULL), _mroResolved(NULL) {
            assert(type != NULL);
        }

        ~MroResolver() {
            Py_CLEAR(_mro);
            Py_CLEAR(_mroResolved);
        }

        // result: mro ordered ((original_class, typeVars), (original_class typeVars))
        PyObject* Resolve() {
            if (InitMro()) {
                return _mroResolved;
            }
            return NULL;
        }

    private:
        inline bool InitMro() {
            Py_CLEAR(_mro);

            if (IsGenericType(_type)) {
                PyObject* origin = GenericTypeOrigin(_type);
                if (origin == NULL) {
                    return false;
                }

                PyObject* origionMro = reinterpret_cast<PyTypeObject*>(origin)->tp_mro;
				assert(origionMro != NULL);
				assert(PyTuple_CheckExact(origionMro));

                _mro = origionMro;
            } else {
                _mro = reinterpret_cast<const PyTypeObject*>(_type)->tp_mro;
            }

            if (_mro == NULL) {
                return false;
            } else {
                Py_INCREF(_mro);

                _mroResolved = PyTuple_New(PyTuple_GET_SIZE(_mro));
                if (_mroResolved == NULL) {
                    return false;
                }

                return CollectMro(_type) && FillMroHoles();
            }
        }


        bool CollectMro(const PyObject* type) {
            PyPtr<> tvars = PyDict_New();
            if (tvars.IsNull()) {
                return false;
            }

            PyPtr<> args = PyObject_GetAttr(const_cast<PyObject*>(type), Module::State()->STR_ARGS);
            if (args.IsValid()) {
                PyPtr<> origin = PyObject_GetAttr(const_cast<PyObject*>(type), Module::State()->STR_ORIGIN);
                if (origin.IsValid()) {
                    PyPtr<> params = PyObject_GetAttr(origin, Module::State()->STR_PARAMETERS);
                    if (params.IsValid()) {
                        assert(PyTuple_CheckExact(params));
                        assert(PyTuple_CheckExact(args));
                        assert(PyTuple_GET_SIZE(args) == PyTuple_GET_SIZE(params));
                        Py_ssize_t l = PyTuple_GET_SIZE(params);

                        if (_typeVars) {
                            for (Py_ssize_t i = 0; i < l; ++i) {
                                PyObject* arg = PyDict_GetItem(const_cast<PyObject*>(_typeVars), PyTuple_GET_ITEM(args, i));
                                if (arg == NULL) {
                                    arg = PyTuple_GET_ITEM(args, i);
                                }

                                if (PyDict_SetItem(tvars, PyTuple_GET_ITEM(params, i), arg) == -1) {
                                    return false;
                                }
                            }
                        } else {
                            for (Py_ssize_t i = 0; i < l; ++i) {
                                if (PyDict_SetItem(tvars, PyTuple_GET_ITEM(params, i), PyTuple_GET_ITEM(args, i)) == -1) {
                                    return false;
                                }
                            }
                        }
                    } else {
                        PyErr_Clear();
                    }
                } else {
                    PyErr_Clear();
                }
            } else {
                PyErr_Clear();
            }

            return CollectMro(type, tvars);
        }


        bool CollectMro(const PyObject* type, PyObject* typeVars) {
            PyPtr<> origin = PyObject_GetAttr(const_cast<PyObject*>(type), Module::State()->STR_ORIGIN);
            if (origin.IsValid()) {
                PyPtr<> bases = PyObject_GetAttr(origin, Module::State()->STR_ORIG_BASES);
                if (bases.IsValid()) {
                    assert(PyTuple_CheckExact(bases));

                    PyPtr<> baseArgs(NULL);
                    PyPtr<> baseOrigin(NULL);
                    PyPtr<> baseParams(NULL);
                    PyPtr<> baseVars(NULL);

                    Py_ssize_t l = PyTuple_GET_SIZE(bases);
                    Py_ssize_t k, kk;
                    for (Py_ssize_t i = 0; i < l; ++i) {
                        PyObject* base = PyTuple_GET_ITEM(bases, i);

                        if (IsGenericType(base)) {
                            // init: baseArgs, baseOrigin, baseParams
                            baseArgs = PyObject_GetAttr(base, Module::State()->STR_ARGS);
                            if (baseArgs.IsValid()) {
                                baseOrigin = PyObject_GetAttr(base, Module::State()->STR_ORIGIN);
                                if (baseOrigin.IsValid()) {
                                    baseParams = PyObject_GetAttr(baseOrigin, Module::State()->STR_PARAMETERS);
                                    if (baseParams.IsNull()) {
                                        PyErr_Clear();
                                        continue;
                                    }
                                } else {
                                    return false;
                                }
                            } else {
                                return false;
                            }

                            // init: baseVars
                            baseVars = PyDict_New();
                            if (baseVars.IsNull()) {
                                return false;
                            }

                            assert(PyTuple_CheckExact(baseArgs));
                            assert(PyTuple_CheckExact(baseParams));
                            assert(PyTuple_GET_SIZE(baseArgs) == PyTuple_GET_SIZE(baseParams));

                            kk = PyTuple_GET_SIZE(baseArgs);
                            for (k = 0; k < kk; ++k) {
                                PyObject* mapped = PyDict_GetItem(typeVars, PyTuple_GET_ITEM(baseArgs, k));
                                if (mapped == NULL) {
                                    mapped = PyTuple_GET_ITEM(baseArgs, k);
                                }

                                if (PyDict_SetItem(baseVars, PyTuple_GET_ITEM(baseParams, k), mapped) == -1) {
                                    return false;
                                }
                            }

                            if (!CollectMro(base, baseVars)) {
                                return false;
                            }
                        }
                    }
                    if (!SetMroEntry(origin, typeVars)) {
                        return false;
                    }
                } else {
                    PyErr_Clear();
                }
            } else {
                PyErr_Clear();
            }

            return true;
        }

        bool SetMroEntry(const PyObject* original, const PyObject* vars) {
            assert(Py_REFCNT(_mro) == 1);

            Py_ssize_t l = PyTuple_GET_SIZE(_mro);
            for (Py_ssize_t i = 0; i < l; ++i) {
                PyObject* cv = PyTuple_GET_ITEM(_mro, i);
                if (cv == original) {
                    PyObject* item = PyTuple_GET_ITEM(_mroResolved, i);
                    Py_XDECREF(item);

                    item = PyTuple_Pack(2, const_cast<PyObject*>(original), const_cast<PyObject*>(vars));
                    if (item == NULL) {
                        return false;
                    }

                    PyTuple_SET_ITEM(_mroResolved, i, item);
                }
            }

            return true;
        }

        bool FillMroHoles() {
            Py_ssize_t l = PyTuple_GET_SIZE(_mro);
            for (Py_ssize_t i = 0; i < l; ++i) {
                PyObject* item = PyTuple_GET_ITEM(_mroResolved, i);
                if (item == NULL) {
                    PyObject* tvars = PyDict_New();
                    if (tvars == NULL) {
                        return false;
                    }

                    item = PyTuple_Pack(2, PyTuple_GET_ITEM(_mro, i), tvars);
                    Py_DECREF(tvars);
                    if (item == NULL) {
                        return false;
                    }

                    PyTuple_SET_ITEM(_mroResolved, i, item);
                }
            }
            return true;
        }


        const PyObject* _type;
        const PyObject* _typeVars;
        PyObject* _mro;
        PyObject* _mroResolved;
};


/**
 * params:
 *      type: PyType...
 *      params: {~T: <type of ~T>}
 *
 * returns {~T: <type of ~T>}
 */
// static PyDictObject* ResolveTypeLocals(PyObject* type, PyDictObject* resolved) {
//     assert(IsGenericType(type));
//     PyPtr<PyDictObject> result = (PyDictObject*)PyDict_New();

//     PyPtr<> args = PyObject_GetAttr(type, Module::State()->STR_ARGS);
//     if (args.IsNull()) {
//         return NULL;
//     }

//     PyPtr<> origin = PyObject_GetAttr(type, Module::State()->STR_ORIGIN);
//     if (origin.IsNull()) {
//         return NULL;
//     }

//     PyPtr<> params = PyObject_GetAttr(origin, Module::State()->STR_PARAMETERS);
//     if (params.IsNull()) {
//         return NULL;
//     }

//     assert(PyTuple_CheckExact(args));
//     assert(PyTuple_CheckExact(params));

//     PyObject* typeValue;
//     for (Py_ssize_t i = 0; i < PyTuple_GET_SIZE(params); ++i) {
//         assert(PyTuple_GET_SIZE(args) > i);

//         if (resolved != NULL) {
//             typeValue = PyDict_GetItem((PyObject*)resolved, PyTuple_GET_ITEM(args, i));
//             if (typeValue == NULL) {
//                 typeValue = PyTuple_GET_ITEM(args, i);
//             }
//         } else {
//             typeValue = PyTuple_GET_ITEM(args, i);
//         }

//         if (PyDict_SetItem(result, PyTuple_GET_ITEM(params, i), typeValue) == -1) {
//             return NULL;
//         }
//     }

//     return result.Steal();
// }


/**
 * returns {~T: <type of ~T>}
 */
// static PyDictObject* ResolveTypeLocals(PyObject* type) {
//     assert(IsGenericType(type));
//     return ResolveTypeLocals(type, NULL);
// }


// return borrowed ref
static inline PyObject* UnpackInjectType(PyObject* inject, PyObject* resolved) {
    assert(IsGenericType(inject));
    assert(PyDict_CheckExact(resolved));

    PyObject* args = PyObject_GetAttr(inject, Module::State()->STR_ARGS);
    if (args == NULL) {
        return NULL;
    }

    assert(PyTuple_CheckExact(args));

    PyObject* res = PyDict_GetItem((PyObject*)resolved, PyTuple_GET_ITEM(args, 0));
    if (res == NULL) {
        res = PyTuple_GET_ITEM(args, 0);
    }

    Py_DECREF(args);
    return res;
}


// static bool ResolveTypeAttributes(PyObject* type, PyDictObject* resolved, PyObject* result) {
//     PyPtr<PyObject> annots = NULL;
//     if (IsGenericType(type)) {
//         PyObject* origin = PyObject_GetAttr(type, Module::State()->STR_ORIGIN);
//         if (origin == NULL) {
//             return false;
//         }
//         annots = PyObject_GetAttr(origin, Module::State()->STR_ANNOTATIONS);
//         Py_DECREF(origin);
//     } else {
//         annots = PyObject_GetAttr(type, Module::State()->STR_ANNOTATIONS);
//     }

//     // maybe not have class annotations
//     if (annots.IsNull()) {
//         PyErr_Clear();
//         return true;
//     }

//     PyObject *key, *value;
//     Py_ssize_t pos = 0;
//     while (PyDict_Next(annots, &pos, &key, &value)) {
//         if (IsInjectableAttr(value)) {
//             PyObject* injectType = UnpackInjectType(value, resolved);
//             if (injectType == NULL) {
//                 return false;
//             }

//             if (PyDict_SetItem(result, key, injectType) == -1) {
//                 return false;
//             }
//         }
//     }

//     return true;
// }


/**
 * Resolve type attributes, that marked with Inject
 *
 * returns tuple(local_types, attribute_types)
 *
 * local_types = {~T: ProvidedType}
 * attribute_types = {"attribute_name": ProvidedType}
 *
 * class Database:
 *     config: Inject[Config]
 *
 * result = ({}, {"config": <type 'Config'>})
 *
 * class Something(Generic[T]):
 *     inject: Inject[T]
 *
 * result = ({~T: <type of T>}, {"inject": <type of T>})
 */
// static PyTupleObject* ResolveTypeHints(PyObject* type) {
//     PyPtr<PyTupleObject> result = (PyTupleObject*)PyTuple_New(2);
//     if (result.IsNull()) {
//         return NULL;
//     }

//     PyDictObject* local = NULL;
//     if (IsGenericType(type)) {
//         local = ResolveTypeLocals(type);
//         if (local == NULL) {
//             return NULL;
//         }
//     } else {
//         local = (PyDictObject*)PyDict_New();
//     }
//     PyTuple_SET_ITEM(result, 0, (PyObject*)local);

//     PyObject* attrs = PyDict_New();
//     if (!ResolveTypeAttributes(type, local, attrs)) {
//         Py_DECREF(attrs);
//         return NULL;
//     }
//     PyTuple_SET_ITEM(result, 1, attrs);

//     return result.Steal();
// }


}}

#endif /* CA8FD8DC_E134_145E_1469_7F5D029ABE37 */
