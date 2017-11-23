#ifndef E59C6756_5133_C8FB_12AD_80BD2DE30129
#define E59C6756_5133_C8FB_12AD_80BD2DE30129

namespace ZenoDI {


ValueResolver* ValueResolver::New(PyObject* name, PyObject* id, PyObject* default_value) {
	ValueResolver* self = ValueResolver::Alloc();
	if (self == NULL) {
		return NULL;
	}

	if (name != NULL) { Py_INCREF(name); }
	if (id != NULL) { Py_INCREF(id); }
	if (default_value != NULL) { Py_INCREF(default_value); }

	self->name = name;
	self->id = id;
	self->default_value = default_value;

	return self;
}


PyObject* ValueResolver::Resolve(ValueResolver* self, Injector* injector) {
	Py_RETURN_NONE;
}


void ValueResolver::__dealloc__(ValueResolver* self) {
	Py_CLEAR(self->name);
	Py_CLEAR(self->id);
	Py_CLEAR(self->default_value);
	Py_TYPE(self)->tp_free((PyObject*) self);
}


PyObject* ValueResolver::__repr__(ValueResolver* self) {
	PyObject* idname;
	if (self->id == NULL) {
		idname = Py_None;
		Py_INCREF(idname);
	} else {
		idname = PyObject_GetAttr(self->id, Module::State()->STR_QUALNAME);
		if (idname == NULL) {
			return NULL;
		}
	}

	PyObject* repr;
	if (self->default_value) {
		repr = PyUnicode_FromFormat("<ValueResolver name=%R, id=%S, default=%R>",
			self->name, idname, self->default_value);
	} else {
		repr = PyUnicode_FromFormat("<ValueResolver name=%R, id=%S>",
			self->name, idname);
	}
	Py_DECREF(idname);
	return repr;
}

} // end namespace ZenoDI

#endif /* E59C6756_5133_C8FB_12AD_80BD2DE30129 */
