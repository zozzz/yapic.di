#include "Python.h"

#include "./injector.hpp"
#include "./provider.hpp"
#include "./resolver.hpp"

namespace ZenoDI {

static PyMethodDef Module_methods[] = {
    {NULL, NULL, 0, NULL}
};


static PyModuleDef Module = {
    PyModuleDef_HEAD_INIT,
    "zeno.di",
    "",
    sizeof(ModuleState),
    Module_methods,
    NULL,
    NULL,
    ModuleState::Clear,
    NULL
};

ModuleState* ModuleState::Get() {
    return ModuleState::Get(PyState_FindModule(&Module));
}

ModuleState* ModuleState::Get(PyObject* module) {
    return (ModuleState*) PyModule_GetState(module);
}

} // end namespace ZenoDI




PyMODINIT_FUNC PyInit_di(void) {
    PyObject* module;

    module = PyModule_Create(&ZenoDI::Module);
    if (module == NULL) {
        return NULL;
    }

    if (!ZenoDI::ModuleState::Init(module)) {
        return NULL;
    }

    ZenoDI_AddType(Injector);
    ZenoDI_AddType(Provider);
    ZenoDI_AddType(ProviderExec);
    ZenoDI_AddType(ValueResolver);

    // if (PyType_Ready(&ZenoDI_TypeVar(Injector)) < 0) {
    //     return NULL;
    // }

    // Py_INCREF(&module);
    // PyModule_AddObject(module, "Injector", (PyObject*) &ZenoDI_TypeVar(Injector));
    return module;
}