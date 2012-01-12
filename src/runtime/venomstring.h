#ifndef VENOM_RUNTIME_STRING_H
#define VENOM_RUNTIME_STRING_H

#include <cassert>
#include <cstdlib>
#include <string>

#include <runtime/venomobject.h>

namespace venom {
namespace runtime {

class venom_string : public venom_object {
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
  static backend::FunctionDescriptor* InitDescriptor;
  static backend::FunctionDescriptor* ReleaseDescriptor;
  static backend::FunctionDescriptor* CtorDescriptor;
  static backend::FunctionDescriptor* StringifyDescriptor;

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
  static venom_class_object StringClassTable;

  inline std::string getData() const {
    return data ? std::string(data, size) : "";
  }

  static venom_ret_cell
  init(backend::ExecutionContext* ctx, venom_cell self) {
    venom_string* s = static_cast<venom_string*>(self.asRawObject());
    s->data = NULL;
    s->size = 0;
    return venom_ret_cell(venom_object::Nil);
  }

  static venom_ret_cell
  release(backend::ExecutionContext* ctx, venom_cell self) {
    venom_string* s = static_cast<venom_string*>(self.asRawObject());
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

private:
  char* data;
  size_t size;
};

}
}

#endif /* VENOM_RUNTIME_STRING_H */
