#ifndef A4EE5BF7_0133_C8FB_154C_E6573E408D54
#define A4EE5BF7_0133_C8FB_154C_E6573E408D54

#include <Python.h>

namespace ZenoDI {

// DONT USE ON BORROWED REFERENCES LIKE PyDict_GetItem
/**
 * Usage:
 * PyLocal<Injector> injector(Injector::Alloc());
 * // do something...
 * return injector.Return()
 *
 * OR:
 *
 */
template<typename O>
class PyLocal {

	public:
		inline PyLocal(O* var): _var(var) {  }
		inline PyLocal(PyObject* var): _var((O*) var) { Py_INCREF(var); }
		inline ~PyLocal() { if (_var != NULL) { Py_DECREF(_var); } }
		inline O& operator* () const { return *_var; }
		inline O* operator-> () const { return _var; }

		inline PyLocal<O>& operator= (const PyLocal<O>& other) {
			if (this != &other) {
				Py_DECREF(_var);
				_var = other._var;
			}
			return *this;
		}

		inline PyLocal<O>& operator= (const O& other) {
			if (_var != &other) {
				Py_DECREF(_var);
				_var = other;
			}
			return *this;
		}

		inline explicit operator PyObject* () const { return (PyObject*) _var; }

		inline bool isNull() const { return _var == NULL; }
		inline bool isValid() const { return _var != NULL; }
		inline PyObject* Return() { T* tmp = _var; _var = NULL; return (PyObject*) _var; }
	private:
		O* _var;
};

} // end namespace ZenoDI

#endif /* A4EE5BF7_0133_C8FB_154C_E6573E408D54 */
