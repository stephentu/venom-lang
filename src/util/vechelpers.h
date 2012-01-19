#ifndef VENOM_UTIL_VECHELPERS_H
#define VENOM_UTIL_VECHELPERS_H

#include <vector>

namespace venom {
namespace util {

/** Define helpers vec1 to vec10 */

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
inline std::vector<T> vec3(const T& a0, const T& a1,
                           const T& a2) {
  typename std::vector<T> v;
  v.reserve(3);
  v.push_back(a0);
  v.push_back(a1);
  v.push_back(a2);
  return v;
}

template <typename T>
inline std::vector<T> vec4(const T& a0, const T& a1,
                           const T& a2, const T& a3) {
  typename std::vector<T> v;
  v.reserve(4);
  v.push_back(a0);
  v.push_back(a1);
  v.push_back(a2);
  v.push_back(a3);
  return v;
}

template <typename T>
inline std::vector<T> vec5(const T& a0, const T& a1,
                           const T& a2, const T& a3,
                           const T& a4) {
  typename std::vector<T> v;
  v.reserve(5);
  v.push_back(a0);
  v.push_back(a1);
  v.push_back(a2);
  v.push_back(a3);
  v.push_back(a4);
  return v;
}

template <typename T>
inline std::vector<T> vec6(const T& a0, const T& a1,
                           const T& a2, const T& a3,
                           const T& a4, const T& a5) {
  typename std::vector<T> v;
  v.reserve(6);
  v.push_back(a0);
  v.push_back(a1);
  v.push_back(a2);
  v.push_back(a3);
  v.push_back(a4);
  v.push_back(a5);
  return v;
}

template <typename T>
inline std::vector<T> vec7(const T& a0, const T& a1,
                           const T& a2, const T& a3,
                           const T& a4, const T& a5,
                           const T& a6) {
  typename std::vector<T> v;
  v.reserve(7);
  v.push_back(a0);
  v.push_back(a1);
  v.push_back(a2);
  v.push_back(a3);
  v.push_back(a4);
  v.push_back(a5);
  v.push_back(a6);
  return v;
}

template <typename T>
inline std::vector<T> vec8(const T& a0, const T& a1,
                           const T& a2, const T& a3,
                           const T& a4, const T& a5,
                           const T& a6, const T& a7) {
  typename std::vector<T> v;
  v.reserve(8);
  v.push_back(a0);
  v.push_back(a1);
  v.push_back(a2);
  v.push_back(a3);
  v.push_back(a4);
  v.push_back(a5);
  v.push_back(a6);
  v.push_back(a7);
  return v;
}

template <typename T>
inline std::vector<T> vec9(const T& a0, const T& a1,
                           const T& a2, const T& a3,
                           const T& a4, const T& a5,
                           const T& a6, const T& a7,
                           const T& a8) {
  typename std::vector<T> v;
  v.reserve(9);
  v.push_back(a0);
  v.push_back(a1);
  v.push_back(a2);
  v.push_back(a3);
  v.push_back(a4);
  v.push_back(a5);
  v.push_back(a6);
  v.push_back(a7);
  v.push_back(a8);
  return v;
}

template <typename T>
inline std::vector<T> vec10(const T& a0, const T& a1,
                            const T& a2, const T& a3,
                            const T& a4, const T& a5,
                            const T& a6, const T& a7,
                            const T& a8, const T& a9) {
  typename std::vector<T> v;
  v.reserve(10);
  v.push_back(a0);
  v.push_back(a1);
  v.push_back(a2);
  v.push_back(a3);
  v.push_back(a4);
  v.push_back(a5);
  v.push_back(a6);
  v.push_back(a7);
  v.push_back(a8);
  v.push_back(a9);
  return v;
}

}
}

#endif /* VENOM_UTIL_VECHELPERS_H */
