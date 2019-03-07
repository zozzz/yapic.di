#include "Python.h"

#include "./di.hpp"
#include "./injector.hpp"
#include "./injectable.hpp"
#include "./resolver.hpp"
#include "./kwonly.hpp"

namespace ZenoDI {

} // end namespace ZenoDI


PyMODINIT_FUNC PyInit__di(void) {
    return ZenoDI::Module::Create();
}
