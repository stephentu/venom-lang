#include <backend/vm.h>
#include <runtime/venomlist.h>
#include <util/stl.h>

using namespace std;
using namespace venom::backend;

namespace venom {
namespace runtime {

FunctionDescriptor* const venom_list::InitDescriptor(
    new FunctionDescriptor((void*)init, 1, 0x1, true));

FunctionDescriptor* const venom_list::ReleaseDescriptor(
    new FunctionDescriptor((void*)release, 1, 0x1, true));
FunctionDescriptor* const venom_list::ReleaseRefDescriptor(
    new FunctionDescriptor((void*)releaseRef, 1, 0x1, true));

FunctionDescriptor* const venom_list::CtorDescriptor(
    new FunctionDescriptor((void*)ctor, 1, 0x1, true));

FunctionDescriptor* const venom_list::StringifyIntDescriptor(
    new FunctionDescriptor((void*)stringifyInt, 1, 0x1, true));
FunctionDescriptor* const venom_list::StringifyFloatDescriptor(
    new FunctionDescriptor((void*)stringifyFloat, 1, 0x1, true));
FunctionDescriptor* const venom_list::StringifyBoolDescriptor(
    new FunctionDescriptor((void*)stringifyBool, 1, 0x1, true));
FunctionDescriptor* const venom_list::StringifyRefDescriptor(
    new FunctionDescriptor((void*)stringifyRef, 1, 0x1, true));

FunctionDescriptor* const venom_list::GetDescriptor(
    new FunctionDescriptor((void*)get, 2, 0x1, true));
FunctionDescriptor* const venom_list::GetRefDescriptor(
    new FunctionDescriptor((void*)getRef, 2, 0x1, true));
FunctionDescriptor* const venom_list::SetDescriptor(
    new FunctionDescriptor((void*)set, 3, 0x1, true));
FunctionDescriptor* const venom_list::SetRefDescriptor(
    new FunctionDescriptor((void*)setRef, 3, 0x5, true));
FunctionDescriptor* const venom_list::AppendDescriptor(
    new FunctionDescriptor((void*)append, 2, 0x1, true));
FunctionDescriptor* const venom_list::AppendRefDescriptor(
    new FunctionDescriptor((void*)appendRef, 2, 0x3, true));

FunctionDescriptor* const venom_list::SizeDescriptor(
    new FunctionDescriptor((void*)size, 1, 0x1, true));

venom_class_object*
venom_list::GetListClassTable(ListType listType) {
  assert(ListIntClassTable != NULL);
  assert(ListFloatClassTable != NULL);
  assert(ListBoolClassTable != NULL);
  assert(ListRefClassTable != NULL);
  switch (listType) {
  case IntType: return ListIntClassTable;
  case FloatType: return ListFloatClassTable;
  case BoolType: return ListBoolClassTable;
  case RefType: return ListRefClassTable;
  default: VENOM_NOT_REACHED;
  }
}

venom_class_object* const venom_list::ListIntClassTable(
    CreateListClassTable(IntType));

venom_class_object* const venom_list::ListFloatClassTable(
    CreateListClassTable(FloatType));

venom_class_object* const venom_list::ListBoolClassTable(
    CreateListClassTable(BoolType));

venom_class_object* const venom_list::ListRefClassTable(
    CreateListClassTable(RefType));

venom_class_object*
venom_list::CreateListClassTable(ListType listType) {
  FunctionDescriptor* stringer;
  switch (listType) {
  case IntType: stringer = StringifyIntDescriptor; break;
  case FloatType: stringer = StringifyFloatDescriptor; break;
  case BoolType: stringer = StringifyBoolDescriptor; break;
  case RefType: stringer = StringifyRefDescriptor; break;
  default: VENOM_NOT_REACHED;
  }
  assert(stringer);
  bool ref = listType == RefType;
  return new venom_class_object(
      "name",
      sizeof(venom_list),
      0, 0x0,
      InitDescriptor,
      ref ? ReleaseRefDescriptor : ReleaseDescriptor,
      CtorDescriptor,
      util::vec5(
        stringer,
        ref ? GetRefDescriptor : GetDescriptor,
        ref ? SetRefDescriptor : SetDescriptor,
        ref ? AppendRefDescriptor : AppendDescriptor,
        SizeDescriptor));
}

}
}
