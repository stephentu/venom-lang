#ifndef VENOM_RUNTIME_REF_H
#define VENOM_RUNTIME_REF_H

#include <runtime/venomobject.h>
#include <util/stl.h>

namespace venom {
namespace runtime {

/**
 * This class is equivalent to the following definition
 * in venom (ignoring the invalid identifier names):
 *
 * class <ref>{T}
 *   attr value::T
 * end
 *
 * Eventually, we will want to replace this with a venom
 * implementation. But since the language is not mature enough
 * to support bootstrapping itself, we implement <ref> as a
 * builtin C++ class instead
 */
template <bool isRefCounted>
class venom_ref_impl : public venom_object {
protected:
  // prevent accidental instantiation/deletion
  venom_ref_impl()  {}
  ~venom_ref_impl() {}
public:
  static venom_class_object& RefClassTable();
};

template <bool isRefCounted>
venom_class_object& venom_ref_impl<isRefCounted>::RefClassTable() {
  static venom_class_object c(
    "<ref>",
    sizeof(venom_ref_impl<isRefCounted>),
    1, isRefCounted ? 0x1 : 0x0,
    &venom_object::InitDescriptor(),
    &venom_object::ReleaseDescriptor(),
    &venom_object::CtorDescriptor(),
    util::vec3(
      &venom_object::StringifyDescriptor(),
      &venom_object::HashDescriptor(),
      &venom_object::EqDescriptor()));
  return c;
}

class venom_ref {
public:
  static inline venom_class_object*
  GetRefClassTable(bool isRefCounted) {
    if (isRefCounted) {
      return &venom_ref_impl<true>::RefClassTable();
    } else {
      return &venom_ref_impl<false>::RefClassTable();
    }
  }
};

}
}

#endif /* VENOM_RUNTIME_REF_H */
