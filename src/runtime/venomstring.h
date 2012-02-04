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

#ifndef VENOM_RUNTIME_STRING_H
#define VENOM_RUNTIME_STRING_H

#include <cassert>
#include <cstring>
#include <cstdlib>
#include <string>

#include <runtime/venomobject.h>

namespace venom {
namespace runtime {

class venom_string : public venom_object,
                     public venom_self_cast<venom_string> {
public:
  /** This constructor is not invocable from user code */
  venom_string(char *data, size_t n)
    : venom_object(&StringClassTable()) { initDataNoCopy(data, n); }

  /** This constructor is not invocable from user code */
  venom_string(const std::string& data)
    : venom_object(&StringClassTable()) { initData(data.data(), data.size()); }

protected:
  ~venom_string() { releaseData(); }

private:
  /**
   * Takes ownership of data.
   * Data MUST have been created via malloc()
   */
  inline void initDataNoCopy(char *data, size_t n) {
    this->data = data;
    this->size = n;
  }

  inline void initData(const char *data, size_t n) {
    this->data = (char *) malloc(n);
    this->size = n;
    assert(this->data);
    memcpy(this->data, data, n);
  }

  inline void releaseData() {
    if (data) {
      free(data);
      data = NULL;
      size = 0;
    }
  }

  static backend::FunctionDescriptor& InitDescriptor();
  static backend::FunctionDescriptor& ReleaseDescriptor();
  static backend::FunctionDescriptor& CtorDescriptor();
  static backend::FunctionDescriptor& StringifyDescriptor();
  static backend::FunctionDescriptor& HashDescriptor();
  static backend::FunctionDescriptor& EqDescriptor();
  static backend::FunctionDescriptor& ConcatDescriptor();

public:
  static venom_class_object& StringClassTable();

  inline std::string getData() const {
    return data ? std::string(data, size) : "";
  }

  static venom_ret_cell
  init(backend::ExecutionContext* ctx, venom_cell self) {
    venom_string* s = asSelf(self);
    s->data = NULL;
    s->size = 0;
    return venom_ret_cell(venom_object::Nil);
  }

  static venom_ret_cell
  release(backend::ExecutionContext* ctx, venom_cell self) {
    venom_string* s = asSelf(self);
    s->releaseData();
    return venom_ret_cell(venom_object::Nil);
  }

  static venom_ret_cell
  ctor(backend::ExecutionContext* ctx, venom_cell self) {
    return venom_ret_cell(venom_object::Nil);
  }

  static venom_ret_cell
  stringify(backend::ExecutionContext* ctx, venom_cell self) {
    return venom_ret_cell(self.asRawObject());
  }

  static venom_ret_cell
  hash(backend::ExecutionContext* ctx, venom_cell self) {
    venom_string* s = asSelf(self);

    // this is java.lang.String's hashCode()
    // http://www.docjar.com/html/api/java/lang/String.java.html
    // TODO: caching the hash
    // TODO: evaluate other alternatives
    int64_t h = 0;
    char* p = s->data;
    size_t i = 0;
    for (; i < s->size; i++, p++) {
      h = 31 * h + int64_t(*p);
    }
    return venom_ret_cell(h);
  }

  static venom_ret_cell
  eq(backend::ExecutionContext* ctx, venom_cell self, venom_cell that) {
    if (that.asRawObject()->getClassObj() != &StringClassTable()) {
      return venom_ret_cell(false);
    }
    venom_string *this_s = asSelf(self);
    venom_string *that_s = asSelf(that);
    if (this_s->size != that_s->size) return venom_ret_cell(false);
    return venom_ret_cell(
        memcmp(this_s->data, that_s->data, this_s->size) == 0);
  }

  static venom_ret_cell
  concat(backend::ExecutionContext* ctx, venom_cell self, venom_cell that) {
    assert(that.asRawObject()->getClassObj() == &StringClassTable());
    venom_string *this_s = asSelf(self);
    venom_string *that_s = asSelf(that);
    size_t total = this_s->size + that_s->size;
    char *data = (char *) malloc(total);
    // TODO: check data
    assert(data);
    memcpy(data, this_s->data, this_s->size);
    memcpy(data + this_s->size, that_s->data, that_s->size);
    return venom_ret_cell(new venom_string(data, total));
  }

private:
  char* data;
  size_t size;
};

}
}

#endif /* VENOM_RUNTIME_STRING_H */
