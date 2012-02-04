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
 * 3. Neither the name of the author nor the names
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

#ifndef VENOM_RUNTIME_BOX_H
#define VENOM_RUNTIME_BOX_H

#include <iostream>
#include <sstream>

#include <backend/vm.h>

#include <runtime/cellutils.h>
#include <runtime/venomobject.h>
#include <runtime/venomstring.h>

namespace venom {
namespace runtime {

template <typename Primitive>
class venom_box_base :
  public venom_object,
  public venom_self_cast< venom_box_base< Primitive > > {

  friend class venom_integer;
  friend class venom_double;
  friend class venom_boolean;
public:

  typedef venom_cell_utils<Primitive> p_utils;

  venom_box_base(Primitive primitive, venom_class_object* class_obj)
    : venom_object(class_obj), primitive(primitive) {}

  static venom_ret_cell
  init(backend::ExecutionContext* ctx, venom_cell self) {
    return venom_ret_cell(Nil);
  }

  static venom_ret_cell
  release(backend::ExecutionContext* ctx, venom_cell self) {
    return venom_ret_cell(Nil);
  }

  static venom_ret_cell
  ctor(backend::ExecutionContext* ctx, venom_cell self, venom_cell value) {
    typename p_utils::extractor x;
    venom_box_base< Primitive >::asSelf(self)->primitive = x(value);
    return venom_ret_cell(Nil);
  }

protected:

  static backend::FunctionDescriptor& InitDescriptor();
  static backend::FunctionDescriptor& ReleaseDescriptor();
  static backend::FunctionDescriptor& CtorDescriptor();
  static backend::FunctionDescriptor& StringifyDescriptor();
  static backend::FunctionDescriptor& HashDescriptor();
  static backend::FunctionDescriptor& EqDescriptor();

  template <typename T>
  static venom_ret_cell stringify_impl(T elem) {
    typename venom_cell_utils<T>::stringer f;
    return venom_ret_cell(new venom_string(f(elem)));
  }

public:
  static venom_ret_cell
  stringify(backend::ExecutionContext* ctx, venom_cell self) {
    return stringify_impl(
        venom_box_base< Primitive >::asSelf(self)->primitive);
  }

  static venom_ret_cell
  hash(backend::ExecutionContext* ctx, venom_cell self) {
    typename venom_cell_utils<Primitive>::hash f;
    return venom_ret_cell(
        f(venom_box_base< Primitive >::asSelf(self)->primitive));
  }

  static venom_ret_cell
  eq(backend::ExecutionContext* ctx, venom_cell self, venom_cell that) {
    if (self.asRawObject()->getClassObj() !=
        that.asRawObject()->getClassObj()) return venom_ret_cell(false);
    venom_box_base<Primitive>* this_p =
      venom_box_base<Primitive>::asSelf(self);
    venom_box_base<Primitive>* that_p =
      venom_box_base<Primitive>::asSelf(that);
    return this_p->primitive == that_p->primitive;
  }

protected:
  Primitive primitive;
};

// statics implementation

template <typename Primitive>
backend::FunctionDescriptor& venom_box_base<Primitive>::InitDescriptor() {
  static backend::FunctionDescriptor f((void*)init, 1, 0x1, true);
  return f;
}

template <typename Primitive>
backend::FunctionDescriptor& venom_box_base<Primitive>::ReleaseDescriptor() {
  static backend::FunctionDescriptor f((void*)release, 1, 0x1, true);
  return f;
}

template <typename Primitive>
backend::FunctionDescriptor& venom_box_base<Primitive>::CtorDescriptor() {
  static backend::FunctionDescriptor f((void*)ctor, 2, 0x1, true);
  return f;
}

template <typename Primitive>
backend::FunctionDescriptor& venom_box_base<Primitive>::StringifyDescriptor() {
  static backend::FunctionDescriptor f((void*)stringify, 1, 0x1, true);
  return f;
}

template <typename Primitive>
backend::FunctionDescriptor& venom_box_base<Primitive>::HashDescriptor() {
  static backend::FunctionDescriptor f((void*)hash, 1, 0x1, true);
  return f;
}

template <typename Primitive>
backend::FunctionDescriptor& venom_box_base<Primitive>::EqDescriptor() {
  static backend::FunctionDescriptor f((void*)eq, 2, 0x3, true);
  return f;
}

namespace {
  typedef venom_cell::cpp_utils<venom_cell::IntType>::cpp_type
          int_type;
  typedef venom_cell::cpp_utils<venom_cell::FloatType>::cpp_type
          float_type;
  typedef venom_cell::cpp_utils<venom_cell::BoolType>::cpp_type
          bool_type;
} // empty namesapce

class venom_integer : public venom_box_base<int_type> {
public:
  static venom_class_object& IntegerClassTable();
  venom_integer(int_type value) :
    venom_box_base<int_type>(value, &IntegerClassTable()) {}
};

class venom_double : public venom_box_base<float_type> {
public:
  static venom_class_object& DoubleClassTable();
  venom_double(float_type value) :
    venom_box_base<float_type>(value, &DoubleClassTable()) {}
};

class venom_boolean : public venom_box_base<bool_type> {
public:
  static venom_class_object& BooleanClassTable();
  venom_boolean(bool_type value) :
    venom_box_base<bool_type>(value, &BooleanClassTable()) {}
};

}
}

#endif /* VENOM_RUNTIME_BOX_H */
