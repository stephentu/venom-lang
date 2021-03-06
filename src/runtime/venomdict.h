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

  static backend::FunctionDescriptor& InitDescriptor();
  static backend::FunctionDescriptor& ReleaseDescriptor();
  static backend::FunctionDescriptor& CtorDescriptor();
  static backend::FunctionDescriptor& StringifyDescriptor();
  static backend::FunctionDescriptor& HashDescriptor();
  static backend::FunctionDescriptor& EqDescriptor();

  static backend::FunctionDescriptor& GetDescriptor();
  static backend::FunctionDescriptor& SetDescriptor();
  static backend::FunctionDescriptor& SizeDescriptor();

  static venom_class_object& DictClassTable();

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
        typename value_utils::extractor()(it->second));
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

// statics implementation

template <typename Key, typename Value>
backend::FunctionDescriptor& venom_dict_impl<Key, Value>::InitDescriptor() {
  static backend::FunctionDescriptor f((void*)init, 1, 0x1, true);
  return f;
}

template <typename Key, typename Value>
backend::FunctionDescriptor& venom_dict_impl<Key, Value>::ReleaseDescriptor() {
  static backend::FunctionDescriptor f((void*)release, 1, 0x1, true);
  return f;
}

template <typename Key, typename Value>
backend::FunctionDescriptor& venom_dict_impl<Key, Value>::CtorDescriptor() {
  static backend::FunctionDescriptor f((void*)ctor, 1, 0x1, true);
  return f;
}

template <typename Key, typename Value>
backend::FunctionDescriptor& venom_dict_impl<Key, Value>::StringifyDescriptor() {
  static backend::FunctionDescriptor f((void*)stringify, 1, 0x1, true);
  return f;
}

template <typename Key, typename Value>
backend::FunctionDescriptor& venom_dict_impl<Key, Value>::HashDescriptor() {
  static backend::FunctionDescriptor f((void*)hash, 1, 0x1, true);
  return f;
}

template <typename Key, typename Value>
backend::FunctionDescriptor& venom_dict_impl<Key, Value>::EqDescriptor() {
  static backend::FunctionDescriptor f((void*)eq, 2, 0x3, true);
  return f;
}

template <typename Key, typename Value>
backend::FunctionDescriptor& venom_dict_impl<Key, Value>::GetDescriptor() {
  static backend::FunctionDescriptor f(
      (void*)get, 2, key_utils::isRefCounted ? 0x1 : 0x0, true);
  return f;
}

template <typename Key, typename Value>
backend::FunctionDescriptor& venom_dict_impl<Key, Value>::SetDescriptor() {
  static backend::FunctionDescriptor f((void*)set, 3,
      (value_utils::isRefCounted ? 0x2 : 0x0) |
        (key_utils::isRefCounted ? 0x1 : 0x0),
      true);
  return f;
}

template <typename Key, typename Value>
backend::FunctionDescriptor& venom_dict_impl<Key, Value>::SizeDescriptor() {
  static backend::FunctionDescriptor f((void*)size, 1, 0x1, true);
  return f;
}

template <typename Key, typename Value>
venom_class_object& venom_dict_impl<Key, Value>::DictClassTable() {
  static venom_class_object c(
    "map",
    sizeof(self_type),
    0, 0x0, &InitDescriptor(), &ReleaseDescriptor(), &CtorDescriptor(),
    util::vec6(
      &StringifyDescriptor(),
      &HashDescriptor(),
      &EqDescriptor(),
      &GetDescriptor(),
      &SetDescriptor(),
      &SizeDescriptor()));
  return c;
}

class venom_dict {
public:
  static venom_class_object*
  GetDictClassTable(venom_cell::CellType keyType,
                    venom_cell::CellType valueType);
};

}
}

#endif /* VENOM_RUNTIME_DICT_H */
