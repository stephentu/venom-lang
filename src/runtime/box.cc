#include <runtime/box.h>

using namespace std;
using namespace venom::backend;

namespace venom {
namespace runtime {

venom_class_object& venom_integer::IntegerClassTable() {
  static venom_class_object c(
    "<Int>",
    sizeof(venom_integer),
    0, 0x0, &InitDescriptor(), &ReleaseDescriptor(), &CtorDescriptor(),
    util::vec3(&StringifyDescriptor(), &HashDescriptor(), &EqDescriptor()));
  return c;
}

venom_class_object& venom_double::DoubleClassTable() {
  static venom_class_object c(
    "<Float>",
    sizeof(venom_double),
    0, 0x0, &InitDescriptor(), &ReleaseDescriptor(), &CtorDescriptor(),
    util::vec3(&StringifyDescriptor(), &HashDescriptor(), &EqDescriptor()));
  return c;
}

venom_class_object& venom_boolean::BooleanClassTable() {
  static venom_class_object c(
    "<Bool>",
    sizeof(venom_boolean),
    0, 0x0, &InitDescriptor(), &ReleaseDescriptor(), &CtorDescriptor(),
    util::vec3(&StringifyDescriptor(), &HashDescriptor(), &EqDescriptor()));
  return c;
}

}
}
