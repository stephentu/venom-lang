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
  venom_string(const char *data, size_t n)
    : venom_object(&StringClassTable) { initData(data, n); }

  /** This constructor is not invocable from user code */
  venom_string(const std::string& data)
    : venom_object(&StringClassTable) { initData(data.data(), data.size()); }

protected:
  ~venom_string() { releaseData(); }

private:
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

public:
  static backend::FunctionDescriptor* const InitDescriptor;
  static backend::FunctionDescriptor* const ReleaseDescriptor;
  static backend::FunctionDescriptor* const CtorDescriptor;
  static backend::FunctionDescriptor* const StringifyDescriptor;
  static backend::FunctionDescriptor* const HashDescriptor;
  static backend::FunctionDescriptor* const EqDescriptor;

  static venom_class_object StringClassTable;

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
    if (that.asRawObject()->getClassObj() != &StringClassTable) {
      return venom_ret_cell(false);
    }
    venom_string *this_s = asSelf(self);
    venom_string *that_s = asSelf(that);
    if (this_s->size != that_s->size) return venom_ret_cell(false);
    return venom_ret_cell(
        memcmp(this_s->data, that_s->data, this_s->size) == 0);
  }

private:
  char* data;
  size_t size;
};

}
}

#endif /* VENOM_RUNTIME_STRING_H */
