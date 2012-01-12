#include <runtime/box.h>

using namespace std;
using namespace venom::backend;

namespace venom {
namespace runtime {

FunctionDescriptor* const venom_integer::InitDescriptor(
    new FunctionDescriptor((void*)init, 1, 0x1, true));

FunctionDescriptor* const venom_integer::ReleaseDescriptor(
    new FunctionDescriptor((void*)release, 1, 0x1, true));

FunctionDescriptor* const venom_integer::CtorDescriptor(
    new FunctionDescriptor((void*)ctor, 2, 0x1, true));

FunctionDescriptor* const venom_integer::StringifyDescriptor(
    new FunctionDescriptor((void*)stringify, 1, 0x1, true));

venom_class_object venom_integer::IntegerClassTable(
    "<Int>",
    sizeof(venom_integer),
    0, 0x0, InitDescriptor, ReleaseDescriptor, CtorDescriptor,
    util::vec1(StringifyDescriptor));

FunctionDescriptor* const venom_double::InitDescriptor(
    new FunctionDescriptor((void*)init, 1, 0x1, true));

FunctionDescriptor* const venom_double::ReleaseDescriptor(
    new FunctionDescriptor((void*)release, 1, 0x1, true));

FunctionDescriptor* const venom_double::CtorDescriptor(
    new FunctionDescriptor((void*)ctor, 2, 0x1, true));

FunctionDescriptor* const venom_double::StringifyDescriptor(
    new FunctionDescriptor((void*)stringify, 1, 0x1, true));

venom_class_object venom_double::DoubleClassTable(
    "<Float>",
    sizeof(venom_double),
    0, 0x0, InitDescriptor, ReleaseDescriptor, CtorDescriptor,
    util::vec1(StringifyDescriptor));

FunctionDescriptor* const venom_boolean::InitDescriptor(
    new FunctionDescriptor((void*)init, 1, 0x1, true));

FunctionDescriptor* const venom_boolean::ReleaseDescriptor(
    new FunctionDescriptor((void*)release, 1, 0x1, true));

FunctionDescriptor* const venom_boolean::CtorDescriptor(
    new FunctionDescriptor((void*)ctor, 2, 0x1, true));

FunctionDescriptor* const venom_boolean::StringifyDescriptor(
    new FunctionDescriptor((void*)stringify, 1, 0x1, true));

venom_class_object venom_boolean::BooleanClassTable(
    "<Bool>",
    sizeof(venom_boolean),
    0, 0x0, InitDescriptor, ReleaseDescriptor, CtorDescriptor,
    util::vec1(StringifyDescriptor));

}
}
