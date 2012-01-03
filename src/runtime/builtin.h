#ifndef VENOM_RUNTIME_BUILTIN_H
#define VENOM_RUNTIME_BUILTIN_H

#include <backend/vm.h>
#include <runtime/venomobject.h>

namespace venom {
namespace runtime {

venom_object_ptr print(backend::ExecutionContext* ctx, venom_object_ptr arg0);

}
}

#endif /* VENOM_RUNTIME_BUILTIN_H */
