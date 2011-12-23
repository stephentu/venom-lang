#ifndef VENOM_UTIL_STL_H
#define VENOM_UTIL_STL_H

#include <iostream>
#include <sstream>
#include <string>

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

template <typename T>
inline std::vector<T> vec1(const T& a0) {
  typename std::vector<T> v;
  v.reserve(1);
  v.push_back(a0);
  return v;
}

template <typename T>
inline std::vector<T> vec2(const T& a0, const T& a1) {
  typename std::vector<T> v;
  v.reserve(2);
  v.push_back(a0);
  v.push_back(a1);
  return v;
}

template <typename T>
inline std::vector<T> vec3(const T& a0, const T& a1, const T& a2) {
  typename std::vector<T> v;
  v.reserve(3);
  v.push_back(a0);
  v.push_back(a1);
  v.push_back(a2);
  return v;
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

}
}

#endif /* VENOM_UTIL_STL_H */
