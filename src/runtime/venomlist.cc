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

FunctionDescriptor* const venom_list::HashIntDescriptor(
    new FunctionDescriptor((void*)hashInt, 1, 0x1, true));
FunctionDescriptor* const venom_list::HashFloatDescriptor(
    new FunctionDescriptor((void*)hashFloat, 1, 0x1, true));
FunctionDescriptor* const venom_list::HashBoolDescriptor(
    new FunctionDescriptor((void*)hashBool, 1, 0x1, true));
FunctionDescriptor* const venom_list::HashRefDescriptor(
    new FunctionDescriptor((void*)hashRef, 1, 0x1, true));

FunctionDescriptor* const venom_list::EqIntDescriptor(
    new FunctionDescriptor((void*)eqInt, 2, 0x3, true));
FunctionDescriptor* const venom_list::EqFloatDescriptor(
    new FunctionDescriptor((void*)eqFloat, 2, 0x3, true));
FunctionDescriptor* const venom_list::EqBoolDescriptor(
    new FunctionDescriptor((void*)eqBool, 2, 0x3, true));
FunctionDescriptor* const venom_list::EqRefDescriptor(
    new FunctionDescriptor((void*)eqRef, 2, 0x3, true));

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
  FunctionDescriptor* hasher;
  FunctionDescriptor* eqer;
  switch (listType) {
  case IntType:
    stringer = StringifyIntDescriptor;
    hasher   = HashIntDescriptor;
    eqer     = EqIntDescriptor;
    break;
  case FloatType:
    stringer = StringifyFloatDescriptor;
    hasher   = HashFloatDescriptor;
    eqer     = EqFloatDescriptor;
    break;
  case BoolType:
    stringer = StringifyBoolDescriptor;
    hasher   = HashBoolDescriptor;
    eqer     = EqBoolDescriptor;
    break;
  case RefType:
    stringer = StringifyRefDescriptor;
    hasher   = HashRefDescriptor;
    eqer     = EqRefDescriptor;
    break;
  default: VENOM_NOT_REACHED;
  }
  assert(stringer);
  assert(hasher);
  assert(eqer);
  bool ref = listType == RefType;
  return new venom_class_object(
      "name",
      sizeof(venom_list),
      0, 0x0,
      InitDescriptor,
      ref ? ReleaseRefDescriptor : ReleaseDescriptor,
      CtorDescriptor,
      util::vec7(
        stringer,
        hasher,
        eqer,
        ref ? GetRefDescriptor : GetDescriptor,
        ref ? SetRefDescriptor : SetDescriptor,
        ref ? AppendRefDescriptor : AppendDescriptor,
        SizeDescriptor));
}

}
}
