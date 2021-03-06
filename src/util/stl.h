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

#ifndef VENOM_UTIL_STL_H
#define VENOM_UTIL_STL_H

#include <cassert>

#include <algorithm>
#include <iostream>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <vector>
#include <utility>

#include <util/macros.h>
#include <util/vechelpers.h>

namespace venom {
namespace util {

/**
 * Delete all the pointers from begin to end.
 * Don't modify the iterators. At the end of this call,
 * all pointers are invalid
 */
template <typename Iter>
inline void delete_pointers(Iter begin, Iter end) {
  while (begin != end) {
    delete *begin;
    ++begin;
  }
}

template <typename Iter>
inline void delete_key_pointers(Iter begin, Iter end) {
  while (begin != end) {
    delete begin->first;
    ++begin;
  }
}

template <typename Iter>
inline void delete_value_pointers(Iter begin, Iter end) {
  while (begin != end) {
    delete begin->second;
    ++begin;
  }
}

struct indent {
  indent(size_t i) : i(i) {}
  const size_t i;
};

inline std::ostream& operator<<(std::ostream& o, const indent& i) {
  std::string s(i.i * 2, ' ');
  o << s;
  return o;
}

inline std::string MakeString2(const std::string& a0,
                                const std::string& a1) {
  std::string s;
  s.reserve(a0.size() + a1.size());
  s.append(a0);
  s.append(a1);
  return s;
}

inline std::string MakeString3(const std::string& a0,
                                const std::string& a1,
                                const std::string& a2) {
  std::string s;
  s.reserve(a0.size() + a1.size() + a2.size());
  s.append(a0);
  s.append(a1);
  s.append(a2);
  return s;
}

template <typename Iter>
inline std::string join(Iter begin, Iter end, const std::string& sep) {
  std::stringstream buf;
  while (begin != end) {
    buf << *begin;
    if (begin + 1 != end) buf << sep;
    ++begin;
  }
  return buf.str();
}

template <typename Iter>
inline bool is_unique(Iter begin, Iter end) {
  std::set<typename Iter::value_type> seen;
  size_t count = 0;
  while (begin != end) {
    seen.insert(*begin);
    ++begin; ++count;
  }
  assert(seen.size() <= count);
  return seen.size() == count;
}

template <typename T>
inline std::vector<T> flatten_vec(const std::vector< std::vector<T> >& vecs) {
  std::vector<T> result;
  typename std::vector< std::vector<T> >::const_iterator it = vecs.begin();
  for (; it != vecs.end(); ++it) {
    result.reserve(result.size() + (*it).size());
    result.insert(result.end(), (*it).begin(), (*it).end());
  }
  return result;
}

template <typename Iter1, typename Iter2, typename BinaryPredicate>
Iter1 binpred_find_if(Iter1 first1, Iter1 last1, Iter2 first2, BinaryPredicate pred) {
  for (; first1 != last1; first1++, first2++) {
    if (pred(*first1, *first2)) break;
  }
  return first1;
}

/**
 * This is actually std::accumulate
 * TODO: replace?
 */
template <typename Iter, typename Accum, typename BinaryFunctor>
Accum foldl(Iter begin, Iter end, Accum start, BinaryFunctor f) {
  Accum cur = start;
  while (begin != end) {
    cur = f(cur, *begin);
    ++begin;
  }
  return cur;
}

template <typename Iter, typename BinaryFunctor>
typename Iter::value_type reducel(Iter begin, Iter end, BinaryFunctor f) {
  typename Iter::value_type cur = *begin;
  ++begin;
  while (begin != end) {
    cur = f(cur, *begin);
    ++begin;
  }
  return cur;
}

template <typename Iter, typename Functor>
inline std::vector<typename Functor::result_type>
transform_vec(Iter begin, Iter end, Functor f) {
  std::vector<typename Functor::result_type> res;
  res.reserve(end - begin);
  while (begin != end) {
    res.push_back(f(*begin));
    ++begin;
  }
  return res;
}

template <typename InputIter1, typename InputIter2, typename OutputIter>
inline OutputIter zip(InputIter1 first1, InputIter1 last1,
                      InputIter2 first2, OutputIter output) {
  while (first1 != last1) {
    *output++ = std::make_pair(*first1++, *first2++);
  }
  return output;
}

template <typename T>
inline void reserve_extra(T& t, size_t extra) {
  t.reserve(t.size() + extra);
}

template <typename T>
inline void stack_clear(std::stack<T>& s) {
  while (!s.empty()) s.pop();
  assert(s.empty());
}

template <typename T>
inline std::string stringify(const T& t) {
  std::stringstream buf;
  buf << t;
  return buf.str();
}

class cout_redirector {
public:
  cout_redirector(std::streambuf* sb)
    : save(std::cout.rdbuf()) { std::cout.rdbuf(sb); }
  ~cout_redirector() { std::cout.rdbuf(save); }
private:
  std::streambuf* save;
};

/** Useful Typedefs **/
typedef std::vector<std::string>       StrVec;
typedef const std::vector<std::string> ConstStrVec;

/* polymorphic pointer cast functor */
template <typename From, typename To,
          bool AssertNotNull, bool AssertTypeOf>
struct _poly_ptr_cast_functor {
  inline To* operator()(From* from) const {
    if (AssertNotNull) VENOM_ASSERT_NOT_NULL(from);
    if (AssertTypeOf) VENOM_ASSERT_TYPEOF_PTR(To, from);
    return AssertTypeOf ? static_cast<To*>(from) : dynamic_cast<To*>(from);
  }
};

template <typename From, typename To>
struct poly_ptr_cast_functor {
  typedef _poly_ptr_cast_functor<From, To, true, true> checked;
  typedef _poly_ptr_cast_functor<From, To, false, false> unchecked;
};

template <typename T>
struct _stringify_functor_ptr {
  typedef std::string result_type;
  inline std::string operator()(const T* t) const { return t->stringify(); }
};

template <typename T>
struct _stringify_functor_ref {
  typedef std::string result_type;
  inline std::string operator()(const T& t) const { return t.stringify(); }
};

template <typename T>
struct stringify_functor {
  typedef _stringify_functor_ptr<T> ptr;
  typedef _stringify_functor_ref<T> ref;
};

/** Debugging helpers **/

/**
 * Generate a string from a collection of pointers to
 * stringify-able objects
 */
template <typename Iter>
inline std::string debug_stringify_ptr_coll(
    Iter begin, Iter end, const std::string& sep) {
  std::stringstream buf;
  while (begin != end) {
    buf << (*begin)->stringify();
    if (begin + 1 != end) buf << sep;
    ++begin;
  }
  return buf.str();
}

}
}

#endif /* VENOM_UTIL_STL_H */
