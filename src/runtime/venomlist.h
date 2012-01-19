#ifndef VENOM_RUNTIME_LIST_H
#define VENOM_RUNTIME_LIST_H

#include <cassert>
#include <sstream>
#include <string>
#include <vector>

#include <runtime/venomobject.h>
#include <runtime/stringifyfunctor.h>

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
class venom_list :
  public venom_object,
  public venom_self_cast<venom_list> {

  friend class backend::Instruction;

protected:
  /** Construct an empty list */
  venom_list(venom_class_object* class_obj) : venom_object(class_obj) {
    elems.reserve(16);
  }

  /** protected destructor, to prevent accidental deletes */
  ~venom_list() {}

public:
  static backend::FunctionDescriptor* const InitDescriptor;
  static backend::FunctionDescriptor* const ReleaseDescriptor;
  static backend::FunctionDescriptor* const ReleaseRefDescriptor;
  static backend::FunctionDescriptor* const CtorDescriptor;

  static backend::FunctionDescriptor* const StringifyIntDescriptor;
  static backend::FunctionDescriptor* const StringifyFloatDescriptor;
  static backend::FunctionDescriptor* const StringifyBoolDescriptor;
  static backend::FunctionDescriptor* const StringifyRefDescriptor;

  static backend::FunctionDescriptor* const HashIntDescriptor;
  static backend::FunctionDescriptor* const HashFloatDescriptor;
  static backend::FunctionDescriptor* const HashBoolDescriptor;
  static backend::FunctionDescriptor* const HashRefDescriptor;

  static backend::FunctionDescriptor* const EqIntDescriptor;
  static backend::FunctionDescriptor* const EqFloatDescriptor;
  static backend::FunctionDescriptor* const EqBoolDescriptor;
  static backend::FunctionDescriptor* const EqRefDescriptor;

  static backend::FunctionDescriptor* const GetDescriptor;
  static backend::FunctionDescriptor* const GetRefDescriptor;
  static backend::FunctionDescriptor* const SetDescriptor;
  static backend::FunctionDescriptor* const SetRefDescriptor;
  static backend::FunctionDescriptor* const AppendDescriptor;
  static backend::FunctionDescriptor* const AppendRefDescriptor;

  static backend::FunctionDescriptor* const SizeDescriptor;

  enum ListType {
    IntType,
    FloatType,
    BoolType,
    RefType,
  };

  /**
   * We use ListType here instead of an analysis::Type instance,
   * so that runtime doesn't have to depend on analysis.
   * NOTE: the venom_class_object instance does NOT need to be
   * freed by the caller
   */
  static venom_class_object* GetListClassTable(ListType listType);

  static venom_ret_cell
  init(backend::ExecutionContext* ctx, venom_cell self) {
    // must use placement new on elems
    using namespace std;
    new (&(asSelf(self)->elems)) vector<venom_cell>();
    return venom_ret_cell(venom_object::Nil);
  }

  static venom_ret_cell
  release(backend::ExecutionContext* ctx, venom_cell self) {
    // must manually call dtor on elems
    using namespace std;
    asSelf(self)->elems.~vector<venom_cell>();
    return venom_ret_cell(venom_object::Nil);
  }

  static venom_ret_cell
  releaseRef(backend::ExecutionContext* ctx, venom_cell self) {
    std::vector<venom_cell>& elems = asSelf(self)->elems;
    for (std::vector<venom_cell>::iterator it = elems.begin();
         it != elems.end(); ++it) {
      it->decRef();
    }
    // must manually call dtor on elems
    using namespace std;
    elems.~vector<venom_cell>();
    return venom_ret_cell(venom_object::Nil);
  }

  static venom_ret_cell
  ctor(backend::ExecutionContext* ctx, venom_cell self) {
    asSelf(self)->elems.reserve(16);
    return venom_ret_cell(venom_object::Nil);
  }

private:
  template <typename T>
  static inline venom_ret_cell
  stringify_impl(backend::ExecutionContext* ctx, venom_cell self) {
    venom_cell::ExtractFunctor<T> extractFunctor;
    venom_stringify_functor<T> stringFunctor;
    std::stringstream buf;
    buf << "[";
    venom_list* list = asSelf(self);
    for (std::vector<venom_cell>::iterator it = list->elems.begin();
         it != list->elems.end(); ++it) {
      buf << stringFunctor(extractFunctor(*it));
      if (it + 1 != list->elems.end()) buf << ", ";
    }
    buf << "]";
    return venom_ret_cell(new venom_string(buf.str()));
  }

  template <typename T>
  static inline venom_ret_cell
  hash_impl(backend::ExecutionContext* ctx, venom_cell self) {
    VENOM_UNIMPLEMENTED;
  }

  template <typename T>
  static inline venom_ret_cell
  eq_impl(backend::ExecutionContext* ctx, venom_cell self, venom_cell that) {
    VENOM_UNIMPLEMENTED;
  }

public:
  static venom_ret_cell
  stringifyInt(backend::ExecutionContext* ctx, venom_cell self) {
    return stringify_impl<int64_t>(ctx, self);
  }

  static venom_ret_cell
  stringifyFloat(backend::ExecutionContext* ctx, venom_cell self) {
    return stringify_impl<double>(ctx, self);
  }

  static venom_ret_cell
  stringifyBool(backend::ExecutionContext* ctx, venom_cell self) {
    return stringify_impl<bool>(ctx, self);
  }

  static venom_ret_cell
  stringifyRef(backend::ExecutionContext* ctx, venom_cell self) {
    return stringify_impl<venom_object*>(ctx, self);
  }

  static venom_ret_cell
  hashInt(backend::ExecutionContext* ctx, venom_cell self) {
    return hash_impl<int64_t>(ctx, self);
  }

  static venom_ret_cell
  hashFloat(backend::ExecutionContext* ctx, venom_cell self) {
    return hash_impl<double>(ctx, self);
  }

  static venom_ret_cell
  hashBool(backend::ExecutionContext* ctx, venom_cell self) {
    return hash_impl<bool>(ctx, self);
  }

  static venom_ret_cell
  hashRef(backend::ExecutionContext* ctx, venom_cell self) {
    return hash_impl<venom_object*>(ctx, self);
  }

  static venom_ret_cell
  eqInt(backend::ExecutionContext* ctx, venom_cell self, venom_cell that) {
    return eq_impl<int64_t>(ctx, self, that);
  }

  static venom_ret_cell
  eqFloat(backend::ExecutionContext* ctx, venom_cell self, venom_cell that) {
    return eq_impl<double>(ctx, self, that);
  }

  static venom_ret_cell
  eqBool(backend::ExecutionContext* ctx, venom_cell self, venom_cell that) {
    return eq_impl<bool>(ctx, self, that);
  }

  static venom_ret_cell
  eqRef(backend::ExecutionContext* ctx, venom_cell self, venom_cell that) {
    return eq_impl<venom_object*>(ctx, self, that);
  }

  static venom_ret_cell
  get(backend::ExecutionContext* ctx, venom_cell self, venom_cell idx) {
    return venom_ret_cell(asSelf(self)->elems.at(idx.asInt()));
  }

  static venom_ret_cell
  getRef(backend::ExecutionContext* ctx, venom_cell self, venom_cell idx) {
    venom_cell elem = asSelf(self)->elems.at(idx.asInt());
    return venom_ret_cell(elem.asRawObject()); // trigger the incRef
  }

  static venom_ret_cell
  set(backend::ExecutionContext* ctx, venom_cell self,
      venom_cell idx, venom_cell elem) {
    asSelf(self)->elems.at(idx.asInt()) = elem;
    return venom_ret_cell(venom_object::Nil);
  }

  static venom_ret_cell
  setRef(backend::ExecutionContext* ctx, venom_cell self,
         venom_cell idx, venom_cell elem) {
    asSelf(self)->elems.at(idx.asInt()) = elem;
    elem.incRef();
    return venom_ret_cell(venom_object::Nil);
  }

  static venom_ret_cell
  append(backend::ExecutionContext* ctx, venom_cell self, venom_cell elem) {
    asSelf(self)->elems.push_back(elem);
    return venom_ret_cell(venom_object::Nil);
  }

  static venom_ret_cell
  appendRef(backend::ExecutionContext* ctx, venom_cell self, venom_cell elem) {
    asSelf(self)->elems.push_back(elem);
    elem.incRef();
    return venom_ret_cell(venom_object::Nil);
  }

  static venom_ret_cell
  size(backend::ExecutionContext* ctx, venom_cell self) {
    return venom_ret_cell(int64_t(asSelf(self)->elems.size()));
  }

private:
  static venom_class_object* const ListIntClassTable;
  static venom_class_object* const ListFloatClassTable;
  static venom_class_object* const ListBoolClassTable;
  static venom_class_object* const ListRefClassTable;

  static venom_class_object* CreateListClassTable(ListType listType);

protected:
  std::vector<venom_cell> elems;
};

}
}

#endif /* VENOM_RUNTIME_LIST_H */
