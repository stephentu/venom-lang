#include <iostream>

#include <runtime/builtin.h>

using namespace std;
using namespace venom::backend;

namespace venom {
namespace runtime {

venom_object_ptr print(ExecutionContext* ctx, venom_object_ptr arg0) {

  return venom_object::NilPtr;
}

}
}
