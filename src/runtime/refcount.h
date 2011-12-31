#ifndef VENOM_RUNTIME_REFCOUNT_H
#define VENOM_RUNTIME_REFCOUNT_H

#include <cassert>

namespace venom {
namespace runtime {

// These two classes are based heavily off of:
// https://github.com/facebook/hiphop-php/blob/master/src/runtime/base/util/countable.h
// https://github.com/facebook/hiphop-php/blob/master/src/runtime/base/util/smart_ptr.h

class venom_countable {
public:
  venom_countable() : count(0) {}
  inline void incRef() const { count++; }
  uint32_t decRef() const { assert(count > 0); return --count; }
protected:
  mutable uint32_t count;
};

/** Ptr must derived from venom_countable,
 * or supply incRef() and decRef() methods */
template <typename Ptr>
class ref_ptr {
public:
  explicit ref_ptr() : ptr(NULL) {}
  explicit ref_ptr(Ptr *ptr) : ptr(ptr) { if (ptr) ptr->incRef(); }

  ref_ptr(const ref_ptr<Ptr>& that) : ptr(that.ptr) { if (ptr) ptr->incRef(); }
  template <typename Derived>
  ref_ptr(const ref_ptr<Derived>& that) : ptr(that.ptr) {
    if (ptr) ptr->incRef();
  }

  ~ref_ptr() { if (ptr && !ptr->decRef()) delete ptr; }

  ref_ptr& operator=(const ref_ptr<Ptr>& that) { return operator=(that.ptr); }
  template <typename Derived>
  ref_ptr& operator=(const ref_ptr<Derived>& that) { return operator=(that.ptr); }
  ref_ptr& operator=(Ptr* ptr) {
    if (this->ptr != ptr) {
      if (this->ptr && !this->ptr->decRef()) delete this->ptr;
      this->ptr = ptr;
      if (this->ptr) this->ptr->incRef();
    }
    return *this;
  }

  Ptr* operator->() const {
    assert(ptr); // null pointer exception?
    return ptr;
  }

  inline Ptr* get() const { return ptr; }
  inline void reset() { operator=((Ptr*)NULL); }

protected:
  Ptr* ptr;
};

}
}

#endif /* VENOM_RUNTIME_REFCOUNT_H */
