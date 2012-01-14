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
class venom_box_base :
  public venom_object,
  public venom_self_cast< venom_box_base< Primitive > > {

  friend class venom_integer;
  friend class venom_double;
  friend class venom_boolean;
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
public:
  static backend::FunctionDescriptor* const InitDescriptor;
  static backend::FunctionDescriptor* const ReleaseDescriptor;
  static backend::FunctionDescriptor* const CtorDescriptor;
  static backend::FunctionDescriptor* const StringifyDescriptor;

  static venom_class_object IntegerClassTable;
  venom_integer(int64_t value) :
    venom_box_base<int64_t>(value, &IntegerClassTable) {}

  static venom_ret_cell
  ctor(backend::ExecutionContext* ctx, venom_cell self, venom_cell value) {
    asSelf(self)->primitive = value.asInt();
    return venom_ret_cell(Nil);
  }

  static venom_ret_cell
  stringify(backend::ExecutionContext* ctx, venom_cell self) {
    std::stringstream buf;
    buf << asSelf(self)->primitive;
    return venom_ret_cell(new venom_string(buf.str()));
  }
};

class venom_double : public venom_box_base<double> {
public:
  static backend::FunctionDescriptor* const InitDescriptor;
  static backend::FunctionDescriptor* const ReleaseDescriptor;
  static backend::FunctionDescriptor* const CtorDescriptor;
  static backend::FunctionDescriptor* const StringifyDescriptor;

  static venom_class_object DoubleClassTable;
  venom_double(double value) :
    venom_box_base<double>(value, &DoubleClassTable) {}

  static venom_ret_cell
  ctor(backend::ExecutionContext* ctx, venom_cell self, venom_cell value) {
    asSelf(self)->primitive = value.asDouble();
    return venom_ret_cell(Nil);
  }

  static venom_ret_cell
  stringify(backend::ExecutionContext* ctx, venom_cell self) {
    std::stringstream buf;
    double value = asSelf(self)->primitive;
    // TODO: HACK, so that 0 as a float gets displayed as 0.0
    if (value) buf << value;
    else buf << "0.0";
    return venom_ret_cell(new venom_string(buf.str()));
  }
};

class venom_boolean : public venom_box_base<bool> {
public:
  static backend::FunctionDescriptor* const InitDescriptor;
  static backend::FunctionDescriptor* const ReleaseDescriptor;
  static backend::FunctionDescriptor* const CtorDescriptor;
  static backend::FunctionDescriptor* const StringifyDescriptor;

  static venom_class_object BooleanClassTable;
  venom_boolean(bool value) :
    venom_box_base<bool>(value, &BooleanClassTable) {}

  static venom_ret_cell
  ctor(backend::ExecutionContext* ctx, venom_cell self, venom_cell value) {
    asSelf(self)->primitive = value.asBool();
    return venom_ret_cell(Nil);
  }

  static venom_ret_cell
  stringify(backend::ExecutionContext* ctx, venom_cell self) {
    std::stringstream buf;
    buf << (asSelf(self)->primitive ? "True" : "False");
    return venom_ret_cell(new venom_string(buf.str()));
  }
};

}
}

#endif /* VENOM_RUNTIME_BOX_H */
