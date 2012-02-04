/**
 * Copyright (c) 2012 Stephen Tu
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the PostgreSQL Global Development Group nor the names
 * of its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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
