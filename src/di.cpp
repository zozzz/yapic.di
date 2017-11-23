#include "Python.h"

#include "./di.hpp"
#include "./injector.hpp"
#include "./provider.hpp"
#include "./resolver.hpp"

namespace ZenoDI {

} // end namespace ZenoDI


PyMODINIT_FUNC PyInit_di(void) {
    return ZenoDI::Module::Create();
}