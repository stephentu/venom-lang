#ifndef VENOM_RUNTIME_BOX_H
#define VENOM_RUNTIME_BOX_H

#include <iostream>
#include <sstream>

#include <backend/vm.h>
#include <runtime/venomobject.h>
#include <runtime/venomstring.h>

namespace venom {
namespace runtime {

template <typename Primitive>
class venom_box_base : public venom_object {
public:
  venom_box_base(Primitive primitive, venom_class_object* class_obj)
    : venom_object(class_obj), primitive(primitive) {}

  static venom_ret_cell
  init(backend::ExecutionContext* ctx, venom_cell self) {
    return venom_ret_cell(Nil);
  }

  static venom_ret_cell
  release(backend::ExecutionContext* ctx, venom_cell self) {
    return venom_ret_cell(Nil);
  }
protected:
  Primitive primitive;
};

class venom_integer : public venom_box_base<int64_t> {
private:
  static backend::FunctionDescriptor* InitDescriptor;
  static backend::FunctionDescriptor* ReleaseDescriptor;
  static backend::FunctionDescriptor* StringifyDescriptor;
public:
  static venom_class_object IntegerClassTable;
  venom_integer(int64_t value) :
    venom_box_base<int64_t>(value, &IntegerClassTable) {}

  static venom_ret_cell
  stringify(backend::ExecutionContext* ctx, venom_cell self) {
    std::stringstream buf;
    buf << static_cast<venom_integer*>(self.asRawObject())->primitive;
    return venom_ret_cell(new venom_string(buf.str()));
  }
};

class venom_double : public venom_box_base<double> {
private:
  static backend::FunctionDescriptor* InitDescriptor;
  static backend::FunctionDescriptor* ReleaseDescriptor;
  static backend::FunctionDescriptor* StringifyDescriptor;
public:
  static venom_class_object DoubleClassTable;
  venom_double(double value) :
    venom_box_base<double>(value, &DoubleClassTable) {}

  static venom_ret_cell
  stringify(backend::ExecutionContext* ctx, venom_cell self) {
    std::stringstream buf;
    double value = static_cast<venom_double*>(self.asRawObject())->primitive;
    // TODO: HACK, so that 0 as a float gets displayed as 0.0
    if (value) buf << value;
    else buf << "0.0";
    return venom_ret_cell(new venom_string(buf.str()));
  }
};

class venom_boolean : public venom_box_base<bool> {
private:
  static backend::FunctionDescriptor* InitDescriptor;
  static backend::FunctionDescriptor* ReleaseDescriptor;
  static backend::FunctionDescriptor* StringifyDescriptor;
public:
  static venom_class_object BooleanClassTable;
  venom_boolean(bool value) :
    venom_box_base<bool>(value, &BooleanClassTable) {}

  static venom_ret_cell
  stringify(backend::ExecutionContext* ctx, venom_cell self) {
    std::stringstream buf;
    buf <<
      (static_cast<venom_boolean*>(self.asRawObject())->primitive ?
       "True" : "False");
    return venom_ret_cell(new venom_string(buf.str()));
  }
};

}
}

#endif /* VENOM_RUNTIME_BOX_H */
