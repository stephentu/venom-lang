#include <backend/vm.h>
#include <runtime/venomstring.h>

using namespace std;
using namespace venom::backend;

namespace venom {
namespace runtime {

FunctionDescriptor* const venom_string::InitDescriptor(
    new FunctionDescriptor((void*)init, 1, 0x1, true));

FunctionDescriptor* const venom_string::ReleaseDescriptor(
    new FunctionDescriptor((void*)release, 1, 0x1, true));

FunctionDescriptor* const venom_string::CtorDescriptor(
    new FunctionDescriptor((void*)ctor, 1, 0x1, true));

FunctionDescriptor* const venom_string::StringifyDescriptor(
    new FunctionDescriptor((void*)stringify, 1, 0x1, true));

FunctionDescriptor* const venom_string::HashDescriptor(
    new FunctionDescriptor((void*)hash, 1, 0x1, true));

FunctionDescriptor* const venom_string::EqDescriptor(
    new FunctionDescriptor((void*)eq, 2, 0x3, true));

venom_class_object venom_string::StringClassTable(
    "string",
    sizeof(venom_string),
    0, 0x0, InitDescriptor, ReleaseDescriptor, CtorDescriptor,
    util::vec3(StringifyDescriptor, HashDescriptor, EqDescriptor));

}
}
