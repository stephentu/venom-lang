#ifndef VENOM_RUNTIME_STRINGIFYFUNCTOR_H
#define VENOM_RUNTIME_STRINGIFYFUNCTOR_H

#include <sstream>
#include <string>

#include <backend/vm.h>
#include <runtime/venomobject.h>
#include <runtime/venomstring.h>

#include <util/macros.h>

namespace venom {
namespace runtime {

template <typename T>
struct venom_stringify_functor {
  inline int64_t hash(T elem) const { return int64_t(elem); }
  inline std::string operator()(T elem) const {
    std::stringstream buf;
    buf << elem;
    return buf.str();
  }
};

template <>
struct venom_stringify_functor<double> {
  inline int64_t hash(double elem) const {
    if (sizeof(double) >= sizeof(int64_t)) {
      // use the bytes of elem as the hash, if we can
      double* p = &elem;
      int64_t* ip = (int64_t*) p;
      return *ip;
    }
    // less ideal hash
    return int64_t(elem);
  }
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
  inline int64_t hash(bool elem) const { return elem ? 1 : 0; }
  inline std::string operator()(bool elem) const {
    std::stringstream buf;
    buf << (elem ? "True" : "False");
    return buf.str();
  }
};

template <>
struct venom_stringify_functor<venom_object*> {
  inline int64_t hash(venom_object* elem) const {
    venom_ret_cell h =
      // 1 is hash vtable entry
      elem->virtualDispatch(backend::ExecutionContext::current_context(), 1);
    return h.asInt();
  }
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
