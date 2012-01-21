#include <runtime/venomdict.h>

namespace venom {
namespace runtime {

venom_class_object*
venom_dict::GetDictClassTable(venom_cell::CellType keyType,
                              venom_cell::CellType valueType) {
  // there are 4 * 4 = 16 possibilities here

#define _INNER_CASE(keyctype, celltype) \
  case venom_cell::celltype: { \
    typedef venom_cell::cpp_utils<venom_cell::celltype>::cpp_type \
            value_cpp_type; \
    return &venom_dict_impl<keyctype, value_cpp_type>::DictClassTable(); \
  }

#define _IMPL_VALUE_TYPE_SWITCH(keyctype) \
  do { \
    switch (valueType) { \
    _INNER_CASE(keyctype, IntType) \
    _INNER_CASE(keyctype, FloatType) \
    _INNER_CASE(keyctype, BoolType) \
    _INNER_CASE(keyctype, RefType) \
    default: VENOM_NOT_REACHED; \
    } \
  } while (0)

#define _CASE(celltype) \
  case venom_cell::celltype: { \
    typedef venom_cell::cpp_utils<venom_cell::celltype>::cpp_type \
            key_cpp_type; \
    _IMPL_VALUE_TYPE_SWITCH(key_cpp_type); \
  }

  switch (keyType) {
  _CASE(IntType)
  _CASE(FloatType)
  _CASE(BoolType)
  _CASE(RefType)
  default: VENOM_NOT_REACHED;
  }

#undef _CASE
#undef _IMPL_VALUE_TYPE_SWITCH
#undef _INNER_CASE

  VENOM_NOT_REACHED;
}

}
}
