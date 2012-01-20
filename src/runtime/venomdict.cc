#include <runtime/venomdict.h>

namespace venom {
namespace runtime {

venom_class_object*
venom_dict::GetDictClassTable(venom_cell::CellType keyType,
                              venom_cell::CellType valueType) {
  // there are 4 * 4 = 16 possibilities here

#define _IMPL_VALUE_TYPE_SWITCH(keyctype) \
  do { \
    switch (valueType) { \
    case venom_cell::IntType: \
      return &venom_dict_impl<keyctype, int64_t>::DictClassTable; \
    case venom_cell::FloatType: \
      return &venom_dict_impl<keyctype, double>::DictClassTable; \
    case venom_cell::BoolType:  \
      return &venom_dict_impl<keyctype, bool>::DictClassTable; \
    case venom_cell::RefType: \
      return &venom_dict_impl<keyctype, venom_object*>::DictClassTable; \
    default: VENOM_NOT_REACHED; \
    } \
  } while (0)

  switch (keyType) {
  case venom_cell::IntType:
    _IMPL_VALUE_TYPE_SWITCH(int64_t);
  case venom_cell::FloatType:
    _IMPL_VALUE_TYPE_SWITCH(double);
  case venom_cell::BoolType:
    _IMPL_VALUE_TYPE_SWITCH(bool);
  case venom_cell::RefType:
    _IMPL_VALUE_TYPE_SWITCH(venom_object*);
  default: VENOM_NOT_REACHED;
  }

#undef _IMPL_VALUE_TYPE_SWITCH

  VENOM_NOT_REACHED;
}

}
}
