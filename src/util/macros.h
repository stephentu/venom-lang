#ifndef VENOM_UTIL_MACROS_H
#define VENOM_UTIL_MACROS_H

#include <iostream>
#include <stdexcept>
#include <string>

#ifdef NDEBUG
  #define venom_pointer_cast static_cast
#else
  #define venom_pointer_cast dynamic_cast
#endif

#if defined(__GNUC__)
  #define ATTRIBUTE_UNUSED __attribute__ ((unused))
#else
  #define ATTRIBUTE_UNUSED
#endif

#define VENOM_NELEMS(array) (sizeof((array))/sizeof((array)[0]))

#define VENOM_STRINGIFY(x) #x

/**
 * __LINE__ comes from the preprocessor as an int.
 * Stringify hack comes from:
 * http://www.decompile.com/cpp/faq/file_and_line_error_string.htm
 */
#define _LINEHACK(x) VENOM_STRINGIFY(x)

#define VENOM_SOURCE_INFO \
  (std::string(__PRETTY_FUNCTION__) + \
   std::string(" (" __FILE__ ":" _LINEHACK(__LINE__) ")"))

#define VENOM_ASSERT(expr) \
  do { \
    typeof (expr) _expr = (expr); \
    if (!_expr) assert(_expr); \
  } while (0)

#define VENOM_ASSERT_NOT_NULL(ptr) VENOM_ASSERT(ptr)

#define VENOM_ASSERT_NULL(ptr) VENOM_ASSERT(!(ptr))

#define VENOM_SAFE_RETURN(array, idx) \
  do { \
    size_t _n = VENOM_NELEMS((array)); \
    size_t _i = static_cast<size_t>((idx)); \
    if (_i < _n) return (array)[_i]; \
    throw std::out_of_range(VENOM_SOURCE_INFO); \
  } while (0)

#define VENOM_CHECK_RANGE(idx, nelems) \
  do { \
    if ((idx) >= (nelems)) { \
      throw std::runtime_error( \
          std::string("Out of range: ") + \
          std::string(VENOM_SOURCE_INFO)); \
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

#define VENOM_SAFE_SET2(treeKid0, treeKid1, param, idx) \
  do { \
    switch (idx) { \
    case 0: VENOM_SAFE_SET_EXPR(treeKid0, param); break; \
    case 1: VENOM_SAFE_SET_EXPR(treeKid1, param); break; \
    default: assert(false); \
    } \
  } while (0)

#define VENOM_SAFE_SET3(treeKid0, treeKid1, treeKid2, param, idx) \
  do { \
    switch (idx) { \
    case 0: VENOM_SAFE_SET_EXPR(treeKid0, param); break; \
    case 1: VENOM_SAFE_SET_EXPR(treeKid1, param); break; \
    case 2: VENOM_SAFE_SET_EXPR(treeKid2, param); break; \
    default: assert(false); \
    } \
  } while (0)

#define VENOM_UNIMPLEMENTED \
  (throw std::runtime_error( \
      std::string("Unimplemented: ") + \
      std::string(VENOM_SOURCE_INFO)))

#define VENOM_NOT_REACHED \
  (throw std::runtime_error( \
      std::string("Should not be reached: ") + \
      std::string(VENOM_SOURCE_INFO)))

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

// This hack is necessary to get the identifier generated
// correctly with VENOM_IDGEN
#define _CONCAT0(a, b) a ## b
#define _CONCAT(a, b) _CONCAT0(a, b)

#define VENOM_COMPILE_TIME_ASSERT(pred) \
  struct _CONCAT(venom_ct_assert_, VENOM_IDGEN) { \
    void asserter() { \
      switch (pred) { case 0: case (pred): default: break; } \
    } \
  }

/** Requires RTTI + type to be polymorphic */
#define VENOM_ASSERT_TYPEOF(type, expr) \
  do { \
    if (false) { \
      type _t ATTRIBUTE_UNUSED = static_cast<type>((expr)); \
    } \
    assert(dynamic_cast<type>((expr))); \
  } while (0)

#define VENOM_ASSERT_TYPEOF_PTR(type, expr) \
  VENOM_ASSERT_TYPEOF(type*, expr)

#ifdef DO_TRACE
  #define VENOM_TRACE(e) std::cerr << "TRACE: " << (e) << std::endl
#else
  #define VENOM_TRACE(e)
#endif

#endif /* VENOM_UTIL_MACROS_H */
