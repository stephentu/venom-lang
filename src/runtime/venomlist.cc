#include <backend/vm.h>
#include <runtime/venomlist.h>
#include <util/stl.h>

using namespace std;
using namespace venom::backend;

namespace venom {
namespace runtime {

venom_class_object*
venom_list::GetListClassTable(venom_cell::CellType listType) {
  switch (listType) {

#define _CASE(celltype) \
  case venom_cell::celltype: { \
    typedef venom_cell::cpp_utils<venom_cell::celltype>::cpp_type cpp_type; \
    return &venom_list_impl<cpp_type>::ListClassTable(); \
  }

  _CASE(IntType)
  _CASE(FloatType)
  _CASE(BoolType)
  _CASE(RefType)

#undef _CASE

  default: VENOM_NOT_REACHED;
  }
}

}
}
