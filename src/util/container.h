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

#ifndef VENOM_UTIL_CONTAINER_H
#define VENOM_UTIL_CONTAINER_H

#include <cassert>
#include <algorithm>
#include <map>
#include <vector>

namespace venom {
namespace util {

template <typename SearchType, typename StoreType>
struct container_base {
  typedef std::vector<StoreType> vec_type;
  typedef std::map<SearchType, size_t> map_type;

  vec_type vec;
  map_type map;

  template <typename Functor>
  size_t createImpl(const SearchType& t, Functor f, bool& create) {
    typename map_type::iterator it = map.find(t);
    if (it == map.end()) {
      size_t ret = vec.size();
      vec.push_back(f(t));
      map[t] = ret;
      create = true;
      return ret;
    } else {
      create = false;
      return it->second;
    }
  }

  bool find(const SearchType& t, size_t& result) {
    typename map_type::iterator it = map.find(t);
    if (it == map.end()) return false;
    result = it->second;
    return true;
  }

  inline void reset() {
    vec.clear();
    map.clear();
  }
};

template <typename SearchType>
struct container_pool_functor {
  inline SearchType operator()(const SearchType& t) const {
    return t;
  }
};

template <typename SearchType>
struct container_pool : public container_base<SearchType, SearchType> {
  inline size_t create(const SearchType& t, bool& create) {
    return createImpl(t, container_pool_functor<SearchType>(), create);
  }
};

template <typename Obj>
class SizedArray {
public:
  typedef Obj* array_type;

  SizedArray()
    : elems(NULL), n_elems(0) {}

  template <typename Iter>
  SizedArray(Iter begin, Iter end)
    : elems(NULL), n_elems(0) {
    n_elems = end - begin;
    elems = new Obj [n_elems];
    std::copy(begin, end, elems);
  }

  ~SizedArray() { if (elems) delete [] elems; }

  SizedArray(const SizedArray<Obj>& that)
    : elems(that.n_elems ? new Obj[that.n_elems] : NULL),
      n_elems(that.n_elems) {
    if (elems) std::copy(that.begin(), that.end(), elems);
  }

  inline SizedArray<Obj>& operator=(const SizedArray<Obj>& that) {
    assert(elems != that.elems); // this shouldn't happen
    if (elems) delete [] elems;
    if (that.elems) {
      elems = new Obj [that.n_elems];
      std::copy(that.begin(), that.end(), elems);
    } else elems = NULL;
    n_elems = that.n_elems;
    return *this;
  }

  static inline SizedArray<Obj> BuildFrom(const std::vector<Obj>& elems) {
    return SizedArray<Obj>(elems.begin(), elems.end());
  }

  inline array_type begin() { return elems; }
  inline const array_type begin() const { return elems; }

  inline array_type end() { return elems + n_elems; }
  inline const array_type end() const { return elems + n_elems; }

  inline size_t size() const { return n_elems; }

private:
  array_type elems;
  size_t n_elems;
};

}
}

#endif /* VENOM_UTIL_CONTAINER_H */
