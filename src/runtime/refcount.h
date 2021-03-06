/**
 * Copyright (c) 2012 Stephen Tu
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names
 * of its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef VENOM_RUNTIME_REFCOUNT_H
#define VENOM_RUNTIME_REFCOUNT_H

#include <cassert>
#include <util/noncopyable.h>

namespace venom {
namespace runtime {

// These two classes are based heavily off of:
// https://github.com/facebook/hiphop-php/blob/master/src/runtime/base/util/countable.h
// https://github.com/facebook/hiphop-php/blob/master/src/runtime/base/util/smart_ptr.h

class venom_countable {
public:
  venom_countable() : count(0) {}
  ~venom_countable() { assert(!count); }
  inline void incRef() const { count++; }
  uint32_t decRef() const { assert(count > 0); return --count; }
  inline uint32_t getCount() const { return count; }
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

/** Does NOT do any automatic object destruction, even if ref
 * count goes to zero */
template <typename Ptr>
class scoped_ref_counter : private util::noncopyable {
public:
  scoped_ref_counter(Ptr *ptr) : ptr(ptr) { ptr->incRef(); }
  ~scoped_ref_counter() { ptr->decRef(); }
private:
  Ptr* ptr;
};

/** Does NOT incRef, but will decRef when it goes out of scope */
template <typename Ptr>
class scoped_ret_value : private util::noncopyable {
public:
  scoped_ret_value(Ptr *ptr) : ptr(ptr) {}
  ~scoped_ret_value() { if (ptr && !ptr->decRef()) delete ptr; }
  Ptr* operator->() const {
    assert(ptr); // null pointer exception?
    return ptr;
  }
  inline Ptr* get() const { return ptr; }
private:
  Ptr* ptr;
};

}
}

#endif /* VENOM_RUNTIME_REFCOUNT_H */
