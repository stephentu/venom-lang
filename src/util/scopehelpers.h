#ifndef VENOM_UTIL_SCOPE_HELPERS_H
#define VENOM_UTIL_SCOPE_HELPERS_H

#include <cassert>

namespace venom {
namespace util {

/**
 * Sets ref to be value for the lifetime of ScopedVariable, and
 * restores ref to be its current value when ScopedVariable dies
 */
template <typename T>
class ScopedVariable {
public:
  ScopedVariable(T& ref, const T& value)
    : oldValue(ref), ref(ref) { ref = value; }
  ~ScopedVariable() { ref = oldValue; }
private:
  T oldValue;
  T& ref;
};

class ScopedBoolean : ScopedVariable<bool> {
public:
  ScopedBoolean(bool& ref) : ScopedVariable<bool>(ref, true) {}
};

}
}

#endif /* VENOM_UTIL_SCOPE_HELPERS_H */
