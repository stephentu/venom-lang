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

class venom_cell {
public:
  /** Order matters */
  enum CellType {
    NoneType = 0,
    IntType,
    DoubleType,
    BoolType,
    ObjType,
  };

  /** Un-initialized ctor */
  venom_cell() : data(0), type(NoneType) {}

  venom_cell(int64_t int_value)
    : data(int_value), type(IntType) {}
  venom_cell(int int_value)
    : data(int_value), type(IntType) {}
  venom_cell(double double_value)
    : data(double_value), type(DoubleType) {}
  venom_cell(float double_value)
    : data(double_value), type(DoubleType) {}
  venom_cell(bool bool_value)
    : data(bool_value), type(BoolType) {}
  venom_cell(venom_object* obj);
  venom_cell(const venom_cell& that);

  ~venom_cell();

  venom_cell& operator=(const venom_cell& that);

  inline bool isInitialized() const { return type; }
  inline bool isPrimitive() const { return type && type < ObjType; }

  inline bool isInt() const { return type == IntType; }
  inline bool isDouble() const { return type == DoubleType; }
  inline bool isBool() const { return type == BoolType; }
  inline bool isObject() const { return type == ObjType; }

  inline int64_t asInt() const { assert(isInt()); return data.int_value; }
  inline double asDouble() const { assert(isDouble()); return data.double_value; }
  inline bool asBool() const { assert(isBool()); return data.bool_value; }
  inline venom_object* asRawObject() const { assert(isObject()); return data.obj; }

  ref_ptr<venom_object> asObject() const;
  ref_ptr<venom_object> box() const;

  inline bool truthTest() const {
    assert(isInitialized());
    if (VENOM_LIKELY(type == BoolType)) return data.bool_value;
    switch (type) {
    case IntType: return data.int_value;
    case DoubleType: return data.double_value;
    // TODO: watch out for boxed primitives...
    case ObjType: return data.obj;
    default: assert(false);
    }
    return false;
  }
  inline bool falseTest() const { return !truthTest(); }

  /** Operator overloads */

  // NOTE: obj operator overload is implemented by invoking the object's
  // operator method, instead of using a C++ operator overload.
  // The C++ operator overloads are to be used for primitive types only.

#define IMPL_OPERATOR_OVERLOAD(op) \
  bool operator op (const venom_cell& that) const { \
    assert(isInitialized()); \
    switch (type) { \
    case IntType: return that.isInt() ? data.int_value op that.asInt() : false; \
    case DoubleType: return that.isDouble() ? data.double_value op that.asDouble() : false; \
    case BoolType: return that.isBool() ? data.bool_value op that.asBool() : false; \
    default: VENOM_NOT_REACHED; \
    } \
  } \

  IMPL_OPERATOR_OVERLOAD(==)
  IMPL_OPERATOR_OVERLOAD(!=)
  IMPL_OPERATOR_OVERLOAD(<)
  IMPL_OPERATOR_OVERLOAD(<=)
  IMPL_OPERATOR_OVERLOAD(>)
  IMPL_OPERATOR_OVERLOAD(>=)

#undef IMPL_OPERATOR_OVERLOAD

  /**
   * Stringify this cell into an STL string. For objects, stringify() will call
   * into the object's virtual stringify() method only if the method is a
   * native one.
   */
  std::string stringifyNativeOnly() const;

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

  /** What type is in data? 0 (NoneType) means not initialized */
  uint8_t type;
};

inline std::ostream& operator<<(std::ostream& o, const venom_cell& cell) {
  o << cell.stringifyNativeOnly();
  return o;
}

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
                     const std::vector<backend::FunctionDescriptor*>& vtable)
    : name(name), sizeof_obj_base(sizeof_obj_base),
      n_cells(n_cells), vtable(vtable) {}

  /** Fully qualified class name (ie a.b.c) */
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
  static ref_ptr<venom_object> NilPtr;

private:
  static backend::FunctionDescriptor* InitDescriptor;
  static backend::FunctionDescriptor* ReleaseDescriptor;
  static backend::FunctionDescriptor* StringifyDescriptor;

public:
  static venom_class_object ObjClassTable;

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
  static ref_ptr<venom_object>
  init(backend::ExecutionContext* ctx, ref_ptr<venom_object> self) {
    return NilPtr;
  }

  /**
   * Replacement for C++ level destructor
   *
   * Default destructor does nothing
   */
  static ref_ptr<venom_object>
  release(backend::ExecutionContext* ctx, ref_ptr<venom_object> self) {
    return NilPtr;
  }

  /**
   * stringify - returns a venom_string object instance
   */
  static ref_ptr<venom_object>
  stringify(backend::ExecutionContext* ctx, ref_ptr<venom_object> self);

  /**
   * Do virtual dispatch on this object, using the n-th method entry in
   * the vtable. Assumes the arguments are already on the stack (except
   * the "this" pointer)
   */
  ref_ptr<venom_object>
  virtualDispatch(backend::ExecutionContext* ctx, size_t index);

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

}
}

#endif /* VENOM_RUNTIME_OBJECT_H */
