#ifndef VENOM_UTIL_MACROS_H
#define VENOM_UTIL_MACROS_H

#include <stdexcept>

#define VENOM_NELEMS(array) (sizeof((array))/sizeof((array)[0]))

#define VENOM_SAFE_RETURN(array, idx) \
  do { \
    size_t _n = VENOM_NELEMS((array)); \
    size_t _i = static_cast<size_t>((idx)); \
    if (_i < _n) return (array)[_i]; \
    throw std::out_of_range(__func__); \
  } while (0)

#endif /* VENOM_UTIL_MACROS_H */
