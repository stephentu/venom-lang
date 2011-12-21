#ifndef VENOM_UTIL_STL_H
#define VENOM_UTIL_STL_H

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

}
}

#endif /* VENOM_UTIL_STL_H */
