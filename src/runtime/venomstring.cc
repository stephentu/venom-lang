#include <backend/vm.h>
#include <runtime/venomstring.h>

using namespace std;
using namespace venom::backend;

namespace venom {
namespace runtime {

FunctionDescriptor& venom_string::InitDescriptor() {
  static FunctionDescriptor f((void*)init, 1, 0x1, true);
  return f;
}

FunctionDescriptor& venom_string::ReleaseDescriptor() {
  static FunctionDescriptor f((void*)release, 1, 0x1, true);
  return f;
}

FunctionDescriptor& venom_string::CtorDescriptor() {
  static FunctionDescriptor f((void*)ctor, 1, 0x1, true);
  return f;
}

FunctionDescriptor& venom_string::StringifyDescriptor() {
  static FunctionDescriptor f((void*)stringify, 1, 0x1, true);
  return f;
}

FunctionDescriptor& venom_string::HashDescriptor() {
  static FunctionDescriptor f((void*)hash, 1, 0x1, true);
  return f;
}

FunctionDescriptor& venom_string::EqDescriptor() {
  static FunctionDescriptor f((void*)eq, 2, 0x3, true);
  return f;
}

FunctionDescriptor& venom_string::ConcatDescriptor() {
  static FunctionDescriptor f((void*)concat, 2, 0x3, true);
  return f;
}

venom_class_object& venom_string::StringClassTable() {
  static venom_class_object c(
    "string",
    sizeof(venom_string),
    0, 0x0, &InitDescriptor(), &ReleaseDescriptor(), &CtorDescriptor(),
    util::vec4(&StringifyDescriptor(), &HashDescriptor(), &EqDescriptor(),
               &ConcatDescriptor()));
  return c;
}

}
}
