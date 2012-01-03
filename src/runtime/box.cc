#include <runtime/box.h>

using namespace std;
using namespace venom::backend;

namespace venom {
namespace runtime {

FunctionDescriptor* venom_integer::InitDescriptor(
    new FunctionDescriptor((void*)init, 1, true));

FunctionDescriptor* venom_integer::ReleaseDescriptor(
    new FunctionDescriptor((void*)release, 1, true));

FunctionDescriptor* venom_integer::StringifyDescriptor(
    new FunctionDescriptor((void*)stringify, 1, true));

venom_class_object venom_integer::IntegerClassTable(
    "integer",
    sizeof(venom_integer),
    0,
    util::vec3(InitDescriptor, ReleaseDescriptor, StringifyDescriptor));

FunctionDescriptor* venom_double::InitDescriptor(
    new FunctionDescriptor((void*)init, 1, true));

FunctionDescriptor* venom_double::ReleaseDescriptor(
    new FunctionDescriptor((void*)release, 1, true));

FunctionDescriptor* venom_double::StringifyDescriptor(
    new FunctionDescriptor((void*)stringify, 1, true));

venom_class_object venom_double::DoubleClassTable(
    "double",
    sizeof(venom_double),
    0,
    util::vec3(InitDescriptor, ReleaseDescriptor, StringifyDescriptor));

FunctionDescriptor* venom_boolean::InitDescriptor(
    new FunctionDescriptor((void*)init, 1, true));

FunctionDescriptor* venom_boolean::ReleaseDescriptor(
    new FunctionDescriptor((void*)release, 1, true));

FunctionDescriptor* venom_boolean::StringifyDescriptor(
    new FunctionDescriptor((void*)stringify, 1, true));

venom_class_object venom_boolean::BooleanClassTable(
    "boolean",
    sizeof(venom_boolean),
    0,
    util::vec3(InitDescriptor, ReleaseDescriptor, StringifyDescriptor));

}
}
