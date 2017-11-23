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


ZenoDI_PrivateNew(ValueResolver)


static void ValueResolver_dealloc(ValueResolver* self) {
	Py_CLEAR(self->name);
	Py_CLEAR(self->id);
	Py_CLEAR(self->default_value);
	Py_TYPE(self)->tp_free((PyObject*) self);
}

static PyObject* ValueResolver_repr(ValueResolver* self) {
	PyObject* idname = (self->id != NULL ? PyObject_GetAttr(self->id, ModuleState::Get()->str__qualname__) : NULL);
	if (idname != NULL && PyErr_Occurred()) {
		PyErr_Clear();
		idname = self->id;
		Py_XINCREF(idname);
	}
	PyObject* repr = PyUnicode_FromFormat("<ValueResolver name=%R, id=%S, default=%R>",
		self->name, idname, self->default_value);
	Py_XDECREF(idname);
	return repr;
}


PyTypeObject ZenoDI_TypeVar(ValueResolver) = {
	PyVarObject_HEAD_INIT(NULL, 0)
	/* tp_name */ 			"zeno.di.ValueResolver",
	/* tp_basicsize */ 		sizeof(ValueResolver),
	/* tp_itemsize */ 		0,
	/* tp_dealloc */ 		(destructor) ValueResolver_dealloc,
	/* tp_print */ 			0,
	/* tp_getattr */ 		0,
	/* tp_setattr */ 		0,
	/* tp_as_async */ 		0,
	/* tp_repr */ 			(reprfunc) ValueResolver_repr,
	/* tp_as_number */ 		0,
	/* tp_as_sequence */ 	0,
	/* tp_as_mapping */ 	0,
	/* tp_hash  */ 			0,
	/* tp_call */ 			0,
	/* tp_str */ 			0,
	/* tp_getattro */ 		0,
	/* tp_setattro */ 		0,
	/* tp_as_buffer */ 		0,
	/* tp_flags */ 			Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	/* tp_doc */ 			0,
	/* tp_traverse */ 		0,
	/* tp_clear */ 			0,
	/* tp_richcompare */ 	0,
	/* tp_weaklistoffset */ 0,
	/* tp_iter */ 			0,
	/* tp_iternext */ 		0,
	/* tp_methods */ 		0,
	/* tp_members */ 		0,
	/* tp_getset */ 		0,
	/* tp_base */ 			0,
	/* tp_dict */ 			0,
	/* tp_descr_get */ 		0,
	/* tp_descr_set */ 		0,
	/* tp_dictoffset */ 	0,
	/* tp_init */ 			0,
	/* tp_alloc */ 			0,
	/* tp_new */ 			ValueResolver_new,
	/* tp_free */ 			0
};


} // end namespace ZenoDI

#endif /* E59C6756_5133_C8FB_12AD_80BD2DE30129 */
