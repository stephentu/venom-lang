#ifndef VENOM_RUNTIME_DICT_H
#define VENOM_RUNTIME_DICT_H

#include <cassert>
#include <sstream>
#include <string>
#include <utility>

#include <runtime/cellutils.h>
#include <runtime/venomobject.h>

#include <util/hashmap.h>
#include <util/macros.h>

namespace venom {
namespace runtime {

template <typename Key, typename Value>
class venom_dict_impl :
  public venom_object,
  public venom_self_cast< venom_dict_impl<Key, Value> > {

  friend class venom_dict;
private:
  typedef HASHMAP_NAMESPACE::HASHMAP_CLASS<
    venom_cell,
    venom_cell,
    typename venom_cell_utils<Key>::hash,
    typename venom_cell_utils<Key>::equal_to> cell_hash_map;

  typedef venom_dict_impl<Key, Value> self_type;

  typedef venom_cell_utils<Key> key_utils;
  typedef venom_cell_utils<Value> value_utils;

protected:
  /** Construct an empty dict */
  venom_dict_impl(venom_class_object* class_obj) : venom_object(class_obj) {}

  /** protected destructor, to prevent accidental deletes */
  ~venom_dict_impl() {}

  static backend::FunctionDescriptor* const InitDescriptor;
  static backend::FunctionDescriptor* const ReleaseDescriptor;
  static backend::FunctionDescriptor* const CtorDescriptor;
  static backend::FunctionDescriptor* const StringifyDescriptor;
  static backend::FunctionDescriptor* const HashDescriptor;
  static backend::FunctionDescriptor* const EqDescriptor;

  static backend::FunctionDescriptor* const GetDescriptor;
  static backend::FunctionDescriptor* const SetDescriptor;
  static backend::FunctionDescriptor* const SizeDescriptor;

  static venom_class_object DictClassTable;

public:
  static venom_ret_cell
  init(backend::ExecutionContext* ctx, venom_cell self) {
    // must use placement new on elems
    new (&(venom_self_cast<self_type>::asSelf(self)->elems)) cell_hash_map();
    return venom_ret_cell(venom_object::Nil);
  }

  static venom_ret_cell
  release(backend::ExecutionContext* ctx, venom_cell self) {
    typename key_utils::dec_ref key_dec_ref;
    typename value_utils::dec_ref value_dec_ref;

    self_type* dict = venom_self_cast<self_type>::asSelf(self);

    for (typename cell_hash_map::iterator it = dict->elems.begin();
         it != dict->elems.end(); ++it) {
      // key decref
      key_dec_ref(it->first);

      // value decref
      value_dec_ref(it->second);
    }

    // must manually call dtor on elems
    dict->elems.~cell_hash_map();
    return venom_ret_cell(venom_object::Nil);
  }

  static venom_ret_cell
  ctor(backend::ExecutionContext* ctx, venom_cell self) {
    return venom_ret_cell(venom_object::Nil);
  }

  static venom_ret_cell
  stringify(backend::ExecutionContext* ctx, venom_cell self) {
    typename key_utils::stringer key_stringer;
    typename value_utils::stringer value_stringer;

    self_type* dict = venom_self_cast<self_type>::asSelf(self);
    std::stringstream buf;
    buf << "{";
    bool first = true;
    for (typename cell_hash_map::iterator it = dict->elems.begin();
         it != dict->elems.end(); ++it, first = false) {
      if (!first) buf << ", ";

      // key
      buf << key_stringer(it->first);

      buf << " : ";

      // value
      buf << value_stringer(it->second);

    }
    buf << "}";
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
  get(backend::ExecutionContext* ctx, venom_cell self, venom_cell key) {
    self_type* dict = venom_self_cast<self_type>::asSelf(self);
    typename cell_hash_map::iterator it = dict->elems.find(key);
    if (it == dict->elems.end()) {
      // TODO: better error message
      throw std::invalid_argument("No such key");
    }
    return venom_ret_cell(
        venom_cell::ExtractFunctor<Value>()(it->second));
  }

  static venom_ret_cell
  set(backend::ExecutionContext* ctx, venom_cell self,
      venom_cell key, venom_cell value) {

    typename key_utils::inc_ref key_inc_ref;
    typename value_utils::inc_ref value_inc_ref;

    typename value_utils::dec_ref value_dec_ref;

    self_type* dict = venom_self_cast<self_type>::asSelf(self);

    // incRef the value first
    value_inc_ref(value);

    std::pair<typename cell_hash_map::iterator, bool> ret =
      dict->elems.insert(
          std::make_pair(key, value));

    if (ret.second) {
      // new element

      // incRef the key
      key_inc_ref(key);
    } else {
      // old elem

      // decRef the value
      value_dec_ref(ret.first->second);

      // no need to incRef the key, since it already exists
      // in the map
    }

    return venom_ret_cell(venom_object::Nil);
  }

  static venom_ret_cell
  size(backend::ExecutionContext* ctx, venom_cell self) {
    return venom_ret_cell(
        int64_t(venom_self_cast<self_type>::asSelf(self)->elems.size()));
  }

protected:
  cell_hash_map elems;
};

template <typename Key, typename Value>
backend::FunctionDescriptor* const venom_dict_impl<Key, Value>::InitDescriptor(
    new backend::FunctionDescriptor((void*)init, 1, 0x1, true));

template <typename Key, typename Value>
backend::FunctionDescriptor* const venom_dict_impl<Key, Value>::ReleaseDescriptor(
    new backend::FunctionDescriptor((void*)release, 1, 0x1, true));

template <typename Key, typename Value>
backend::FunctionDescriptor* const venom_dict_impl<Key, Value>::CtorDescriptor(
    new backend::FunctionDescriptor((void*)ctor, 1, 0x1, true));

template <typename Key, typename Value>
backend::FunctionDescriptor* const venom_dict_impl<Key, Value>::StringifyDescriptor(
    new backend::FunctionDescriptor((void*)stringify, 1, 0x1, true));

template <typename Key, typename Value>
backend::FunctionDescriptor* const venom_dict_impl<Key, Value>::HashDescriptor(
    new backend::FunctionDescriptor((void*)hash, 1, 0x1, true));

template <typename Key, typename Value>
backend::FunctionDescriptor* const venom_dict_impl<Key, Value>::EqDescriptor(
    new backend::FunctionDescriptor((void*)eq, 2, 0x3, true));

template <typename Key, typename Value>
backend::FunctionDescriptor* const venom_dict_impl<Key, Value>::GetDescriptor(
    new backend::FunctionDescriptor((void*)get, 2, key_utils::isRefCounted ? 0x1 : 0x0, true));

template <typename Key, typename Value>
backend::FunctionDescriptor* const venom_dict_impl<Key, Value>::SetDescriptor(
    new backend::FunctionDescriptor((void*)set, 3,
      (value_utils::isRefCounted ? 0x2 : 0x0) | (key_utils::isRefCounted ? 0x1 : 0x0),
      true));

template <typename Key, typename Value>
backend::FunctionDescriptor* const venom_dict_impl<Key, Value>::SizeDescriptor(
    new backend::FunctionDescriptor((void*)size, 1, 0x1, true));

template <typename Key, typename Value>
venom_class_object venom_dict_impl<Key, Value>::DictClassTable(
    "map",
    sizeof(self_type),
    0, 0x0, InitDescriptor, ReleaseDescriptor, CtorDescriptor,
    util::vec6(
      StringifyDescriptor,
      HashDescriptor,
      EqDescriptor,
      GetDescriptor,
      SetDescriptor,
      SizeDescriptor));

class venom_dict {
public:
  static venom_class_object*
  GetDictClassTable(venom_cell::CellType keyType,
                    venom_cell::CellType valueType);
};

}
}

#endif /* VENOM_RUNTIME_DICT_H */
