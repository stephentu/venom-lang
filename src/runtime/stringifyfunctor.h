#ifndef VENOM_RUNTIME_STRINGIFYFUNCTOR_H
#define VENOM_RUNTIME_STRINGIFYFUNCTOR_H

#include <sstream>
#include <string>

#include <backend/vm.h>
#include <runtime/venomobject.h>
#include <runtime/venomstring.h>

namespace venom {
namespace runtime {

template <typename T>
struct venom_stringify_functor {
  inline std::string operator()(T elem) const {
    std::stringstream buf;
    buf << elem;
    return buf.str();
  }
};

template <>
struct venom_stringify_functor<double> {
  inline std::string operator()(double elem) const {
    std::stringstream buf;
    // TODO: HACK, so that 0 as a float gets displayed as 0.0
    if (elem) buf << elem;
    else buf << "0.0";
    return buf.str();
  }
};

template <>
struct venom_stringify_functor<bool> {
  inline std::string operator()(bool elem) const {
    std::stringstream buf;
    buf << (elem ? "True" : "False");
    return buf.str();
  }
};

template <>
struct venom_stringify_functor<venom_object*> {
  inline std::string operator()(venom_object* elem) const {
    std::stringstream buf;
    venom_ret_cell str =
      // 0 is stringify vtable entry
      elem->virtualDispatch(backend::ExecutionContext::current_context(), 0);
    scoped_ret_value<venom_object> ptr(str.asRawObject());
    assert(ptr->getClassObj() == &venom_string::StringClassTable);
    return static_cast<venom_string*>(ptr.get())->getData();
  }
};

}
}

#endif /* VENOM_RUNTIME_STRINGIFYFUNCTOR_H */
