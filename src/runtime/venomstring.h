#ifndef VENOM_RUNTIME_STRING_H
#define VENOM_RUNTIME_STRING_H

#include <string>
#include <runtime/venomobject.h>

namespace venom {
namespace runtime {

class venom_string : public venom_object {
public:
  /** This constructor is not invocable from user code */
  venom_string(const char *data, size_t n)
    : venom_object(&StringClassTable), data(data, n) {}

  /** This constructor is not invocable from user code */
  venom_string(const std::string& data)
    : venom_object(&StringClassTable), data(data) {}

private:
  static backend::FunctionDescriptor* InitDescriptor;
  static backend::FunctionDescriptor* ReleaseDescriptor;
  static backend::FunctionDescriptor* StringifyDescriptor;

public:
  static venom_class_object StringClassTable;

  inline std::string& getData() { return data; }
  inline const std::string& getData() const { return data; }

  static venom_object_ptr
  init(backend::ExecutionContext* ctx, venom_object_ptr self) {
    new (&(static_cast<venom_string*>(self.get())->data)) std::string();
    return venom_object::NilPtr;
  }

  static venom_object_ptr
  release(backend::ExecutionContext* ctx, venom_object_ptr self) {
    using namespace std;
    static_cast<venom_string*>(self.get())->data.~string();
    return venom_object::NilPtr;
  }

  static venom_object_ptr
  stringify(backend::ExecutionContext* ctx, venom_object_ptr self) {
    return self;
  }

private:
  std::string data;
};

}
}

#endif /* VENOM_RUNTIME_STRING_H */
