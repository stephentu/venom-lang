#ifndef VENOM_RUNTIME_OBJECT_H
#define VENOM_RUNTIME_OBJECT_H

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#include <runtime/refcount.h>
#include <util/macros.h>
#include <util/stl.h>

namespace venom {
namespace runtime {

/** Forward decls */
class venom_object;
class venom_class_object;

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

  ~venom_cell();

  inline bool isInitialized() const { return type; }
  inline bool isPrimitive() const { return type && type < ObjType; }

  inline bool isInt() const { return type == IntType; }
  inline bool isDouble() const { return type == DoubleType; }
  inline bool isBool() const { return type == BoolType; }
  inline bool isObject() const { return type == ObjType; }

  inline int64_t asInt() const { assert(isInt()); return data.int_value; }
  inline double asDouble() const { assert(isDouble()); return data.double_value; }
  inline bool asBool() const { assert(isBool()); return data.bool_value; }

  bool operator==(const venom_cell& that) const {
    switch (type) {
    case NoneType: return that.type == NoneType;
    case IntType: return data.int_value == that.data.int_value;
    case DoubleType: return data.double_value == that.data.double_value;
    case BoolType: return data.bool_value == that.data.bool_value;
    // TODO: implement obj equality (not just pointer equality)
    case ObjType: return data.obj == that.data.obj;
    default: VENOM_NOT_REACHED;
    }
  }
  inline bool operator!=(const venom_cell& that) const {
    return !operator==(that);
  }

  std::string stringify() const {
    switch (type) {
    case NoneType: return "Uninit";
    case IntType: return util::stringify(data.int_value);
    case DoubleType: return util::stringify(data.double_value);
    case BoolType: return util::stringify(data.bool_value);
    // TODO: implement obj stringify
    case ObjType: return !data.obj ? "Nil" : "Obj";
    default: VENOM_NOT_REACHED;
    }

  }

private:
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
  o << cell.stringify();
  return o;
}

/**
 * A venom_object expects to have its n_cells as a contiguous piece of memory
 * immediately following the object's memory. In other words, a venom_object is
 * really a sizeof(venom_object) + n_cells * sizeof(venom_cell) bytes
 * contiguous piece of memory.
 *
 * To allocate a venom_object of N cells:
 *
 *   // allocate the memory
 *   venom_object *obj = (venom_object *) operator new (venom_object_sizeof(N));
 *   // call constructor via placement new
 *   new (obj) venom_object(N);
 *
 * To de-allocate a venom_object obj, delete is sufficient:
 *
 *   delete obj;
 */
class venom_object : public venom_countable {
public:
  static inline size_t venom_object_sizeof(size_t n_cells) {
    return sizeof(venom_object) + n_cells * sizeof(venom_cell);
  }

  venom_object(size_t n_cells) : n_cells(n_cells) {
    // this is equivalent to looping over each of the
    // cells and calling the constructor, but faster
    if (n_cells) memset(cell_ptr(0), 0, n_cells * sizeof(venom_cell));
  }
  ~venom_object() {
    if (n_cells) {
      venom_cell *p = cell_ptr(0);
      // manually call the destructor. we *don't* delete
      // a venom_cell, since we did not allocate it with
      // operator new.
      for (size_t i = 0; i < n_cells; i++, p++) {
        p->~venom_cell();
      }
    }
  }

  inline venom_cell& cell(size_t n) { return *cell_ptr(n); }
  inline const venom_cell& cell(size_t n) const {
    return *const_cast<venom_object*>(this)->cell_ptr(n);
  }

private:
  inline venom_cell* cell_ptr(size_t n) {
    assert(n < n_cells);
    char *p = (char *) this;
    return (venom_cell *)(p + sizeof(venom_object));
  }

  venom_class_object* class_obj;
  size_t n_cells;
};
typedef ref_ptr<venom_object> venom_object_ptr;

class venom_class_object {
public:
  std::string name;
};

}
}

#endif /* VENOM_RUNTIME_OBJECT_H */
