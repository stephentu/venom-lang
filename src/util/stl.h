#ifndef VENOM_UTIL_STL_H
#define VENOM_UTIL_STL_H

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

inline std::string* MakeString2(const std::string& a0,
                                const std::string& a1) {
  std::string *s = new std::string;
  s->reserve(a0.size() + a1.size());
  s->append(a0);
  s->append(a1);
  return s;
}

inline std::string* MakeString3(const std::string& a0,
                                const std::string& a1,
                                const std::string& a2) {
  std::string *s = new std::string;
  s->reserve(a0.size() + a1.size() + a2.size());
  s->append(a0);
  s->append(a1);
  s->append(a2);
  return s;
}

}
}

#endif /* VENOM_UTIL_STL_H */
