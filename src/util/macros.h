#ifndef VENOM_UTIL_MACROS_H
#define VENOM_UTIL_MACROS_H

#include <iostream>
#include <stdexcept>
#include <string>

#if NDEBUG
  #define venom_pointer_cast static_cast
#else
  #define venom_pointer_cast dynamic_cast
#endif

#define VENOM_NELEMS(array) (sizeof((array))/sizeof((array)[0]))

#define VENOM_SAFE_RETURN(array, idx) \
  do { \
    size_t _n = VENOM_NELEMS((array)); \
    size_t _i = static_cast<size_t>((idx)); \
    if (_i < _n) return (array)[_i]; \
    throw std::out_of_range(__PRETTY_FUNCTION__); \
  } while (0)

#define VENOM_CHECK_RANGE(idx, nelems) \
  do { \
    if ((idx) >= (nelems)) { \
      throw std::runtime_error( \
          std::string("Out of range: ") + \
          std::string(__PRETTY_FUNCTION__)); \
    } \
  } while (0)

#define VENOM_SAFE_SET_EXPR(expr, param) \
  do { \
    if (!param) expr = NULL; \
    else { \
      typeof(expr) _k = venom_pointer_cast<typeof(expr)>(param); \
      assert(_k); \
      expr = _k; \
    } \
  } while (0)

#define VENOM_SAFE_SET_CASE(treeKid, param) \
  do { \
    VENOM_SAFE_SET_EXPR(treeKid, param); \
    break; \
  } while (0)

#define VENOM_SAFE_SET2(treeKid0, treeKid1, param, idx) \
  do { \
    switch (idx) { \
    case 0: VENOM_SAFE_SET_CASE(treeKid0, param); \
    case 1: VENOM_SAFE_SET_CASE(treeKid1, param); \
    default: assert(false); \
    } \
  } while (0)

#define VENOM_SAFE_SET3(treeKid0, treeKid1, treeKid2, param, idx) \
  do { \
    switch (idx) { \
    case 0: VENOM_SAFE_SET_CASE(treeKid0, param); \
    case 1: VENOM_SAFE_SET_CASE(treeKid1, param); \
    case 2: VENOM_SAFE_SET_CASE(treeKid2, param); \
    default: assert(false); \
    } \
  } while (0)

#define VENOM_UNIMPLEMENTED \
  (throw std::runtime_error( \
      std::string("Unimplemented: ") + \
      std::string(__PRETTY_FUNCTION__)))

#define VENOM_NOT_REACHED \
  (throw std::runtime_error( \
      std::string("Should not be reached: ") + \
      std::string(__PRETTY_FUNCTION__)))

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

#define VENOM_TRACE(e) std::cerr << "TRACE: " << (e) << std::endl

#endif /* VENOM_UTIL_MACROS_H */
