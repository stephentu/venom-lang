#include <iostream>

#include <runtime/builtin.h>
#include <runtime/venomstring.h>

using namespace std;
using namespace venom::backend;

namespace venom {
namespace runtime {

FunctionDescriptor* BuiltinPrintDescriptor(
  new FunctionDescriptor((void*)print, 1, true));

ostream venom_stdout(cout.rdbuf());

venom_object_ptr print(ExecutionContext* ctx, venom_object_ptr arg0) {
  venom_object_ptr str = arg0->virtualDispatch(ctx, 2);
  assert(str->getClassObj() == &venom_string::StringClassTable);
  venom_stdout << static_cast<venom_string*>(str.get())->getData() << endl;
  return venom_object::NilPtr;
}

}
}
