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

#ifndef VENOM_RUNTIME_OBJECT_H
#define VENOM_RUNTIME_OBJECT_H

#include <cassert>
#include <cstdio>
#include <cstdlib>

#include <iostream>
#include <string>
#include <vector>

#include <runtime/refcount.h>
#include <util/macros.h>
#include <util/stl.h>

namespace venom {

namespace backend {
  /** Forward decl */
  class ExecutionContext;
  class FunctionDescriptor;
}

namespace runtime {

/** Forward decls */
class venom_object;

/**
 * A cell is a unit of storage on the stack and as an object
 * member field. A cell is *not* aware of its contents- it is really
 * just a union with some helper methods.
 */
class venom_cell {
  friend class venom_ret_cell;
public:
  enum CellType {
    IntType,
    FloatType,
    BoolType,
    RefType,
  };

  /** This should allow us to avoid hardcoding c++ types
   * in various places. See specializations below */
  template <enum CellType>
  struct cpp_utils {
    static const bool is_valid = false;
    typedef void* cpp_type;
  };

  /** Un-initialized ctor */
  venom_cell() : data(0) {}

  venom_cell(int64_t int_value) : data(int_value) {}
  venom_cell(int int_value) : data(int_value) {}
  venom_cell(double double_value) : data(double_value) {}
  venom_cell(float double_value) : data(double_value) {}
  venom_cell(bool bool_value) : data(bool_value) {}
  venom_cell(venom_object* obj) : data(obj) {}

  inline int64_t asInt() const { return data.int_value; }
  inline double asDouble() const { return data.double_value; }
  inline bool asBool() const { return data.bool_value; }
  inline venom_object* asRawObject() const { return data.obj; }
  ref_ptr<venom_object> asObject() const;

  void incRef();
  void decRef();

#ifdef NDEBUG
  static inline void AssertNonZeroRefCount(const venom_cell& cell) {}
#else
  static void AssertNonZeroRefCount(const venom_cell& cell);
#endif

  // use specializations defined below
  template <typename T>
  struct ExtractFunctor {};

protected:
  union types {
    /** Primitives */
    int64_t int_value;
    double double_value;
    bool bool_value;

    /** Everything else */
    venom_object* obj;

    /** union type constructors */
    types(int64_t int_value) : int_value(int_value) {}
    types(int int_value) : int_value(int_value) {}
    types(double double_value) : double_value(double_value) {}
    types(float double_value) : double_value(double_value) {}
    types(bool bool_value) : bool_value(bool_value) {}
    types(venom_object* obj) : obj(obj) {}
  } data;
};

/** Template specializations for venom_cell::cpp_utils */

template <>
struct venom_cell::cpp_utils<venom_cell::IntType> {
  static const bool is_valid = true;
  typedef int64_t cpp_type;
};

template <>
struct venom_cell::cpp_utils<venom_cell::FloatType> {
  static const bool is_valid = true;
  typedef double cpp_type;
};

template <>
struct venom_cell::cpp_utils<venom_cell::BoolType> {
  static const bool is_valid = true;
  typedef bool cpp_type;
};

template <>
struct venom_cell::cpp_utils<venom_cell::RefType> {
  static const bool is_valid = true;
  typedef venom_object* cpp_type;
};

/** Template specializations for venom_cell::ExtractFunctor */

#define _IMPL_EXTRACT_FUNCTOR(ctype, type) \
  template <> \
  struct venom_cell::ExtractFunctor<ctype> { \
    inline ctype operator()(venom_cell cell) const { \
      return cell.as##type(); \
    } \
  };

_IMPL_EXTRACT_FUNCTOR(int64_t, Int)
_IMPL_EXTRACT_FUNCTOR(double, Double)
_IMPL_EXTRACT_FUNCTOR(bool, Bool)
_IMPL_EXTRACT_FUNCTOR(venom_object*, RawObject)
_IMPL_EXTRACT_FUNCTOR(ref_ptr<venom_object>, Object)

#undef _IMPL_EXTRACT_FUNCTOR

/**
 * The same as a cell, except when you construct a venom_ret_cell
 * with a venom_object, an incRef() is done automatically.
 */
class venom_ret_cell : public venom_cell {
public:
  venom_ret_cell(int64_t int_value) : venom_cell(int_value) {}
  venom_ret_cell(int int_value) : venom_cell(int_value) {}
  venom_ret_cell(double double_value) : venom_cell(double_value) {}
  venom_ret_cell(float double_value) : venom_cell(double_value) {}
  venom_ret_cell(bool bool_value) : venom_cell(bool_value) {}
  venom_ret_cell(venom_object* obj) : venom_cell(obj) { incRef(); }

  // TODO: try to use special venom_cell ctor for this
  venom_ret_cell(const venom_cell& cell) { data = cell.data; }
};

/**
 * A venom_class_object is created once per class, and holds the vtable for
 * that class. In the future, when reflection is implemented, it will also
 * contain the necessary data structures to support reflection.
 */
class venom_class_object {
public:
  venom_class_object(const std::string& name,
                     size_t sizeof_obj_base,
                     size_t n_cells,
                     uint64_t ref_cell_bitmap,
                     backend::FunctionDescriptor* cppInit,
                     backend::FunctionDescriptor* cppRelease,
                     backend::FunctionDescriptor* ctor,
                     const std::vector<backend::FunctionDescriptor*>& vtable)
    : name(name), sizeof_obj_base(sizeof_obj_base),
      n_cells(n_cells), ref_cell_bitmap(ref_cell_bitmap),
      cppInit(cppInit), cppRelease(cppRelease), ctor(ctor), vtable(vtable) {
    // TODO: implementation limitation for now
    assert(n_cells <= 64);
  }

  /** Class name */
  const std::string name;

  /**
   * The base size of an object, in bytes. For all user classes, this is simply
   * sizeof(venom_object). Only for builtin classes is this value possibly
   * larger (it should never be smaller than sizeof(venom_object)). Note this
   * size does *NOT* include the cells.
   */
  const size_t sizeof_obj_base;

  /** The number of cells that this object requires */
  const size_t n_cells;

  /** Bitmap which indicates which cells are ref-counted
   * TODO: handle when an obj has > 64 fields */
  const uint64_t ref_cell_bitmap;

  backend::FunctionDescriptor* const cppInit;

  backend::FunctionDescriptor* const cppRelease;

  backend::FunctionDescriptor* const ctor;

  /** The vtable for the class */
  const std::vector<backend::FunctionDescriptor*> vtable;
};

/**
 * A venom_object is the base class for all objects in the venom runtime.
 *
 * A venom_object expects to have its n_cells as a contiguous piece of memory
 * immediately following the object's memory. In other words, a venom_object is
 * really a sizeof(venom_object) + n_cells * sizeof(venom_cell) bytes
 * contiguous piece of memory.
 *
 * To allocate a venom_object of N cells:
 *
 *   // allocate the memory
 *   venom_object *obj =
 *     (venom_object *) operator new (venom_object::venom_object_sizeof(N));
 *   // call constructor via placement new
 *   new (obj) venom_object(N);
 *
 * To de-allocate a venom_object obj, delete is sufficient:
 *
 *   delete obj;
 *
 * A venom_object is not a polymorphic (virtual) class in C++.
 * Instead, a venom_object contains a pointer to its class's
 * venom_class_object, which is used to implement virtual dispatch.
 *
 * TODO: fill this in more completely
 */
class venom_object : public venom_countable {
public:
  static inline size_t
  venom_object_sizeof(size_t obj_base, size_t n_cells) {
    return obj_base + n_cells * sizeof(venom_cell);
  }

  static venom_object* Nil;

  static backend::FunctionDescriptor& InitDescriptor();
  static backend::FunctionDescriptor& ReleaseDescriptor();
  static backend::FunctionDescriptor& CtorDescriptor();
  static backend::FunctionDescriptor& StringifyDescriptor();
  static backend::FunctionDescriptor& HashDescriptor();
  static backend::FunctionDescriptor& EqDescriptor();

  static venom_class_object& ObjClassTable();

  /**
   * Does *NOT* call the venom level ctor, only calls the
   * CPP level init. Also, does not incRef()
   */
  static inline venom_object* allocObj(venom_class_object* class_obj) {
    assert(class_obj);
    size_t s =
      venom_object_sizeof(
          class_obj->sizeof_obj_base, class_obj->n_cells);
    venom_object *obj = (venom_object *) operator new (s);
    new (obj) venom_object(class_obj);
    return obj;
  }

  venom_object(venom_class_object* class_obj);
  ~venom_object();

  inline venom_cell& cell(size_t n) { return *cell_ptr(n); }
  inline const venom_cell& cell(size_t n) const {
    return *const_cast<venom_object*>(this)->cell_ptr(n);
  }

  inline venom_class_object* getClassObj() { return class_obj; }
  inline const venom_class_object* getClassObj() const { return class_obj; }

  std::string stringifyNativeOnly() const;

  /**
   * Base object methods :
   *
   * These are not implemented as C++ methods, since we use our
   * own virtual dispatch mechanism.
   *
   * Consequently, we have to simulate having a virtual destructor also (see
   * release()).
   */

  /**
   * Replacement for C++ level constructor
   *
   * Default constructor does nothing
   */
  static venom_ret_cell
  init(backend::ExecutionContext* ctx, venom_cell self) {
    return venom_ret_cell(Nil);
  }

  /**
   * Replacement for C++ level destructor
   *
   * Default destructor does nothing
   */
  static venom_ret_cell
  release(backend::ExecutionContext* ctx, venom_cell self) {
    return venom_ret_cell(Nil);
  }

  /**
   * Venom level constructor
   *
   * Default constructor does nothing
   */
  static venom_ret_cell
  ctor(backend::ExecutionContext* ctx, venom_cell self) {
    return venom_ret_cell(Nil);
  }

  /**
   * stringify - returns a venom_string object instance
   */
  static venom_ret_cell
  stringify(backend::ExecutionContext* ctx, venom_cell self);

  /**
   * hash - returns an int which is a hash of this instance
   */
  static venom_ret_cell
  hash(backend::ExecutionContext* ctx, venom_cell self) {
    return venom_ret_cell(int64_t(self.asRawObject()));
  }

  /**
   * eq - returns true if parameter is the same instance
   */
  static venom_ret_cell
  eq(backend::ExecutionContext* ctx, venom_cell self, venom_cell that) {
    // pointer equality
    return venom_ret_cell(self.asRawObject() == that.asRawObject());
  }

  /**
   * Do virtual dispatch on this object, using the n-th method entry in
   * the vtable. Args do NOT include the *this* pointer
   */
  venom_ret_cell
  virtualDispatch(
      backend::ExecutionContext* ctx,
      size_t index,
      const std::vector<venom_cell>& args = std::vector<venom_cell>());

  venom_ret_cell dispatchInit(backend::ExecutionContext* ctx);

  venom_ret_cell dispatchRelease(backend::ExecutionContext* ctx);

protected:
  /**
   * return a pointer to the n-th cell of this object
   *
   * TODO: locating a cell pointer requires one pointer deference
   * in the current scheme, which means accessing an attribute in
   * the venom VM requires two pointer deferences instead of one.
   * See if we can come up with a scheme which does not require
   * the extra pointer deference.
   */
  inline venom_cell* cell_ptr(size_t n) {
    assert(n < class_obj->n_cells);
    char *p = (char *) this;
    return (venom_cell *)
      (p + class_obj->sizeof_obj_base + n * sizeof(venom_cell));
  }

  /** vtable ptr + other class information */
  venom_class_object* class_obj;
};
typedef ref_ptr<venom_object> venom_object_ptr;

inline std::ostream& operator<<(std::ostream& o, const venom_object& obj) {
  o << obj.stringifyNativeOnly();
  return o;
}

template <typename T>
class venom_self_cast {
protected:

  // TODO: not sure to take venom_cell by ref, or by value

  static inline T* asSelf(venom_cell self) {
    return static_cast<T*>(self.asRawObject());
  }

  static inline T& asSelfRef(venom_cell self) {
    return *asSelf(self);
  }
};

}
}

#endif /* VENOM_RUNTIME_OBJECT_H */
