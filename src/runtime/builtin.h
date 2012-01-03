#ifndef VENOM_RUNTIME_BUILTIN_H
#define VENOM_RUNTIME_BUILTIN_H

#include <iostream>

#include <backend/vm.h>
#include <runtime/venomobject.h>

namespace venom {
namespace runtime {

extern backend::FunctionDescriptor* BuiltinPrintDescriptor;
extern std::ostream venom_stdout;
venom_object_ptr print(backend::ExecutionContext* ctx, venom_object_ptr arg0);

}
}

#endif /* VENOM_RUNTIME_BUILTIN_H */
