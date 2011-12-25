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

#define SAFE_SELECT(x, y) ((x) ? ((x)->y) : NULL)

#define SAFE_ADDR(x, y) ((x) ? (&((x)->y)) : NULL)

#endif /* VENOM_UTIL_MACROS_H */
