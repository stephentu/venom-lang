#include <iostream>

#include <runtime/builtin.h>
#include <runtime/venomstring.h>

using namespace std;
using namespace venom::backend;

namespace venom {
namespace runtime {

FunctionDescriptor& BuiltinPrintDescriptor() {
  static FunctionDescriptor f((void*)print, 1, 0x1, true);
  return f;
}

ostream venom_stdout(cout.rdbuf());

venom_ret_cell print(ExecutionContext* ctx, venom_cell arg0) {
  venom_ret_cell ret = arg0.asRawObject()->virtualDispatch(ctx, 0);
  scoped_ret_value<venom_object> ptr(ret.asRawObject());
  assert(ptr->getClassObj() == &venom_string::StringClassTable());
  venom_stdout << static_cast<venom_string*>(ptr.get())->getData() << endl;
  return venom_ret_cell(venom_object::Nil);
}

}
}
