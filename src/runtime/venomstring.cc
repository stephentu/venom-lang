#include <backend/vm.h>
#include <runtime/venomstring.h>

using namespace std;
using namespace venom::backend;

namespace venom {
namespace runtime {

FunctionDescriptor* venom_string::InitDescriptor(
    new FunctionDescriptor((void*)init, 1, 0x1, true));

FunctionDescriptor* venom_string::ReleaseDescriptor(
    new FunctionDescriptor((void*)release, 1, 0x1, true));

FunctionDescriptor* venom_string::StringifyDescriptor(
    new FunctionDescriptor((void*)stringify, 1, 0x1, true));

venom_class_object venom_string::StringClassTable(
    "string",
    sizeof(venom_string),
    0, 0,
    util::vec3(InitDescriptor, ReleaseDescriptor, StringifyDescriptor));

}
}
