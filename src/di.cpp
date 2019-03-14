#include "Python.h"

#include "./di.hpp"
#include "./injector.hpp"
#include "./injectable.hpp"
#include "./resolver.hpp"
#include "./kwonly.hpp"

namespace YapicDI {

} // end namespace YapicDI


PyMODINIT_FUNC PyInit__di(void) {
    return YapicDI::Module::Create();
}
