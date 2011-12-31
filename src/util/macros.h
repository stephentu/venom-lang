#ifndef VENOM_UTIL_MACROS_H
#define VENOM_UTIL_MACROS_H

#include <stdexcept>
#include <string>

#define VENOM_NELEMS(array) (sizeof((array))/sizeof((array)[0]))

#define VENOM_SAFE_RETURN(array, idx) \
  do { \
    size_t _n = VENOM_NELEMS((array)); \
    size_t _i = static_cast<size_t>((idx)); \
    if (_i < _n) return (array)[_i]; \
    throw std::out_of_range(__func__); \
  } while (0)

#define VENOM_UNIMPLEMENTED \
  (throw std::runtime_error( \
      std::string("Unimplemented: ") + std::string(__func__)))

#define VENOM_NOT_REACHED \
  (throw std::runtime_error( \
      std::string("Should not be reached: ") + std::string(__func__)))

/**
 * Use forchild as such:
 * forchild (kid) {
 *   // do something with kid
 * } endfor
 */
#define forchild(kid) \
  for (size_t i = 0; i < getNumKids(); i++) { \
    ASTNode *kid = getNthKid(i); \
    {

#define endfor }}

#define VENOM_SAFE_SELECT(x, y) ((x) ? ((x)->y) : NULL)

#define VENOM_SAFE_ADDROF(x, y) ((x) ? (&((x)->y)) : NULL)

#define VENOM_LIKELY(pred) __builtin_expect((pred), true)

#define VENOM_UNLIKELY(pred) __builtin_expect((pred), false)

#ifdef __COUNTER__
  #define VENOM_IDGEN __COUNTER__
#else
  #define VENOM_IDGEN __LINE__
#endif

#define VENOM_COMPILE_TIME_ASSERT(pred) \
  struct venom_ct_assert ## VENOM_IDGEN { \
    void asserter() { \
      switch (pred) { case 0: case (pred): default: break; } \
    } \
  }

#if DEBUG
  #define venom_pointer_cast dynamic_cast
#else
  #define venom_pointer_cast static_cast
#endif

#endif /* VENOM_UTIL_MACROS_H */
