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

#ifndef VENOM_RUNTIME_LIST_H
#define VENOM_RUNTIME_LIST_H

#include <cassert>
#include <sstream>
#include <string>
#include <vector>

#include <runtime/cellutils.h>
#include <runtime/venomobject.h>

#include <util/macros.h>

namespace venom {

namespace backend {
  /** Forward decl */
  class Instruction;
}

namespace runtime {

/**
 * Even though list has a type parameter
 * in the language, the C++ object is not a template
 * type. For now, a list is backed by a std::vector, since
 * this is the most straightforward implementation.
 */
template <typename Elem>
class venom_list_impl :
  public venom_object,
  public venom_self_cast< venom_list_impl<Elem> > {

  friend class backend::Instruction;
  friend class venom_list;

private:
  typedef venom_list_impl<Elem> self_type;
  typedef venom_cell_utils<Elem> elem_utils;

protected:
  /** Construct an empty list */
  venom_list_impl(venom_class_object* class_obj) : venom_object(class_obj) {
    elems.reserve(16);
  }

  /** protected destructor, to prevent accidental deletes */
  ~venom_list_impl() {}

  static backend::FunctionDescriptor& InitDescriptor();
  static backend::FunctionDescriptor& ReleaseDescriptor();
  static backend::FunctionDescriptor& CtorDescriptor();

  static backend::FunctionDescriptor& StringifyDescriptor();
  static backend::FunctionDescriptor& HashDescriptor();
  static backend::FunctionDescriptor& EqDescriptor();

  static backend::FunctionDescriptor& GetDescriptor();
  static backend::FunctionDescriptor& SetDescriptor();
  static backend::FunctionDescriptor& AppendDescriptor();

  static backend::FunctionDescriptor& SizeDescriptor();

  static venom_class_object& ListClassTable();

public:

  static venom_ret_cell
  init(backend::ExecutionContext* ctx, venom_cell self) {
    // must use placement new on elems
    using namespace std;
    new (&(venom_self_cast<self_type>::asSelf(self)->elems))
      vector<venom_cell>();
    return venom_ret_cell(venom_object::Nil);
  }

  static venom_ret_cell
  release(backend::ExecutionContext* ctx, venom_cell self) {
    std::vector<venom_cell>& elems =
      venom_self_cast<self_type>::asSelf(self)->elems;
    if (elem_utils::isRefCounted) {
      for (std::vector<venom_cell>::iterator it = elems.begin();
           it != elems.end(); ++it) {
        it->decRef();
      }
    }

    // must manually call dtor on elems
    using namespace std;
    elems.~vector<venom_cell>();
    return venom_ret_cell(venom_object::Nil);
  }

  static venom_ret_cell
  ctor(backend::ExecutionContext* ctx, venom_cell self) {
    venom_self_cast<self_type>::asSelf(self)->elems.reserve(16);
    return venom_ret_cell(venom_object::Nil);
  }

  static venom_ret_cell
  stringify(backend::ExecutionContext* ctx, venom_cell self) {
    typename elem_utils::stringer stringer;
    std::stringstream buf;
    buf << "[";
    self_type* list = venom_self_cast<self_type>::asSelf(self);
    for (std::vector<venom_cell>::iterator it = list->elems.begin();
         it != list->elems.end(); ++it) {
      buf << stringer(*it);
      if (it + 1 != list->elems.end()) buf << ", ";
    }
    buf << "]";
    return venom_ret_cell(new venom_string(buf.str()));
  }

  static venom_ret_cell
  hash(backend::ExecutionContext* ctx, venom_cell self) {
    VENOM_UNIMPLEMENTED;
  }

  static venom_ret_cell
  eq(backend::ExecutionContext* ctx, venom_cell self, venom_cell that) {
    VENOM_UNIMPLEMENTED;
  }

  static venom_ret_cell
  get(backend::ExecutionContext* ctx, venom_cell self, venom_cell idx) {
    venom_cell elem =
      venom_self_cast<self_type>::asSelf(self)->elems.at(idx.asInt());
    return venom_ret_cell(
        typename elem_utils::extractor()(elem)); // trigger the incRef
  }

  static venom_ret_cell
  set(backend::ExecutionContext* ctx, venom_cell self,
      venom_cell idx, venom_cell elem) {
    // TODO: static?
    typename elem_utils::inc_ref incr;
    typename elem_utils::dec_ref decr;

    self_type* list = venom_self_cast<self_type>::asSelf(self);

    // inc ref new
    incr(elem);

    // dec ref old
    venom_cell old = list->elems.at(idx.asInt());
    decr(old);

    // set
    list->elems.at(idx.asInt()) = elem;

    return venom_ret_cell(venom_object::Nil);
  }

  static venom_ret_cell
  append(backend::ExecutionContext* ctx, venom_cell self, venom_cell elem) {
    venom_self_cast<self_type>::asSelf(self)->elems.push_back(elem);
    typename elem_utils::inc_ref incr;
    incr(elem);
    return venom_ret_cell(venom_object::Nil);
  }

  static venom_ret_cell
  size(backend::ExecutionContext* ctx, venom_cell self) {
    return venom_ret_cell(
        int64_t(venom_self_cast<self_type>::asSelf(self)->elems.size()));
  }

protected:
  std::vector<venom_cell> elems;
};

// static implementations

template <typename Elem>
backend::FunctionDescriptor& venom_list_impl<Elem>::InitDescriptor() {
  static backend::FunctionDescriptor f((void*)init, 1, 0x1, true);
  return f;
}
template <typename Elem>
backend::FunctionDescriptor& venom_list_impl<Elem>::ReleaseDescriptor() {
  static backend::FunctionDescriptor f((void*)release, 1, 0x1, true);
  return f;
}
template <typename Elem>
backend::FunctionDescriptor& venom_list_impl<Elem>::CtorDescriptor() {
  static backend::FunctionDescriptor f((void*)ctor, 1, 0x1, true);
  return f;
}

template <typename Elem>
backend::FunctionDescriptor& venom_list_impl<Elem>::StringifyDescriptor() {
  static backend::FunctionDescriptor f((void*)stringify, 1, 0x1, true);
  return f;
}
template <typename Elem>
backend::FunctionDescriptor& venom_list_impl<Elem>::HashDescriptor() {
  static backend::FunctionDescriptor f((void*)hash, 1, 0x1, true);
  return f;
}
template <typename Elem>
backend::FunctionDescriptor& venom_list_impl<Elem>::EqDescriptor() {
  static backend::FunctionDescriptor f((void*)eq, 2, 0x3, true);
  return f;
}

template <typename Elem>
backend::FunctionDescriptor& venom_list_impl<Elem>::GetDescriptor() {
  static backend::FunctionDescriptor f((void*)get, 2, 0x1, true);
  return f;
}
template <typename Elem>
backend::FunctionDescriptor& venom_list_impl<Elem>::SetDescriptor() {
  static backend::FunctionDescriptor f(
      (void*)set, 3, 0x1 | (elem_utils::isRefCounted ? 0x4 : 0x0), true);
  return f;
}
template <typename Elem>
backend::FunctionDescriptor& venom_list_impl<Elem>::AppendDescriptor() {
  static backend::FunctionDescriptor f(
      (void*)append, 2, 0x1 | (elem_utils::isRefCounted ? 0x2 : 0x0), true);
  return f;
}

template <typename Elem>
backend::FunctionDescriptor& venom_list_impl<Elem>::SizeDescriptor() {
  static backend::FunctionDescriptor f((void*)size, 1, 0x1, true);
  return f;
}

template <typename Elem>
venom_class_object& venom_list_impl<Elem>::ListClassTable() {
  static venom_class_object c(
      "list", sizeof(self_type), 0, 0x0,
      &InitDescriptor(), &ReleaseDescriptor(), &CtorDescriptor(),
      util::vec7(
        &StringifyDescriptor(), &HashDescriptor(), &EqDescriptor(),
        &GetDescriptor(), &SetDescriptor(), &AppendDescriptor(),
        &SizeDescriptor()));
  return c;
}

class venom_list {
public:

  typedef venom_list_impl<
            venom_cell::cpp_utils<venom_cell::IntType>::cpp_type
          > int_list_type;

  typedef venom_list_impl<
            venom_cell::cpp_utils<venom_cell::FloatType>::cpp_type
          > float_list_type;

  typedef venom_list_impl<
            venom_cell::cpp_utils<venom_cell::BoolType>::cpp_type
          > bool_list_type;

  typedef venom_list_impl<
            venom_cell::cpp_utils<venom_cell::RefType>::cpp_type
          > ref_list_type;

  /**
   * We use CellType here instead of an analysis::Type instance,
   * so that runtime doesn't have to depend on analysis.
   * NOTE: the venom_class_object instance does NOT need to be
   * freed by the caller
   */
  static venom_class_object*
  GetListClassTable(venom_cell::CellType listType);
};

}
}

#endif /* VENOM_RUNTIME_LIST_H */
