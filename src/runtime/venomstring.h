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

  static venom_object_ptr
  init(backend::ExecutionContext* ctx, venom_object_ptr self) {
    venom_string* s = static_cast<venom_string*>(self.get());
    s->data = NULL;
    s->size = 0;
    return venom_object::NilPtr;
  }

  static venom_object_ptr
  release(backend::ExecutionContext* ctx, venom_object_ptr self) {
    venom_string* s = static_cast<venom_string*>(self.get());
    s->releaseData();
    return venom_object::NilPtr;
  }

  static venom_object_ptr
  stringify(backend::ExecutionContext* ctx, venom_object_ptr self) {
    return self;
  }

private:
  char* data;
  size_t size;
};

}
}

#endif /* VENOM_RUNTIME_STRING_H */
