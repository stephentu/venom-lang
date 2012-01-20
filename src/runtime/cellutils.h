#ifndef VENOM_RUNTIME_CELLUTILS_H
#define VENOM_RUNTIME_CELLUTILS_H

#include <sstream>
#include <string>

#include <backend/vm.h>
#include <runtime/venomobject.h>
#include <runtime/venomstring.h>

#include <util/macros.h>

namespace venom {
namespace runtime {

namespace {

  template <typename T0>
  struct _ref_count_impl { static const bool value = false; };

  template <>
  struct _ref_count_impl<venom_object*> { static const bool value = true; };

  template <typename T0>
  struct _inc_ref_impl {
    inline void operator()(venom_cell elem) const {}
  };

  template <>
  struct _inc_ref_impl<venom_object*> {
    inline void operator()(venom_cell elem) const { elem.incRef(); }
  };

  template <typename T0>
  struct _dec_ref_impl {
    inline void operator()(venom_cell elem) const {}
  };

  template <>
  struct _dec_ref_impl<venom_object*> {
    inline void operator()(venom_cell elem) const { elem.decRef(); }
  };

  template <typename T0>
  struct _stringer_impl {
    inline std::string operator()(T0 elem) const {
      std::stringstream buf;
      buf << elem;
      return buf.str();
    }
  };

  template <>
  struct _stringer_impl<double> {
    inline std::string operator()(double elem) const {
      std::stringstream buf;
      // TODO: HACK, so that 0 as a float gets displayed as 0.0
      if (elem) buf << elem;
      else buf << "0.0";
      return buf.str();

    }
  };

  template <>
  struct _stringer_impl<bool> {
    inline std::string operator()(bool elem) const {
      std::stringstream buf;
      buf << (elem ? "True" : "False");
      return buf.str();
    }
  };

  template <>
  struct _stringer_impl<venom_object*> {
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

  template <typename T0>
  struct _hash_impl {
    inline int64_t operator()(T0 elem) const { return int64_t(elem); }
  };

  template <>
  struct _hash_impl<double> {
    inline int64_t operator()(double elem) const {
      if (sizeof(double) >= sizeof(int64_t)) {
        // use the bytes of elem as the hash, if we can
        double* p = &elem;
        int64_t* ip = (int64_t*) p;
        return *ip;
      }
      // less ideal hash
      return int64_t(elem);
    }
  };

  template <>
  struct _hash_impl<venom_object*> {
    inline int64_t operator()(venom_object* elem) const {
      venom_ret_cell h =
        // 1 is hash vtable entry
        elem->virtualDispatch(backend::ExecutionContext::current_context(), 1);
      return h.asInt();
    }
  };

  template <typename T0>
  struct _equal_impl {
    inline bool operator()(T0 lhs, T0 rhs) const {
      return lhs == rhs;
    }
  };

  template <>
  struct _equal_impl<venom_object*> {
    inline bool operator()(venom_object* lhs, venom_object* rhs) const {
      // 2 is eq vtable entry
      venom_ret_cell ret =
        lhs->virtualDispatch(backend::ExecutionContext::current_context(), 2);
      return ret.asBool();
    }
  };

}

template <typename T>
struct venom_cell_utils {
  typedef venom_cell::ExtractFunctor<T> extractor;

  static const bool isRefCounted = _ref_count_impl<T>::value;

  struct inc_ref {
    inline void operator()(venom_cell elem) const {
      _inc_ref_impl<T>()(elem);
    }
  };

  struct dec_ref {
    inline void operator()(venom_cell elem) const {
      _dec_ref_impl<T>()(elem);
    }
  };

  struct stringer {
    inline std::string operator()(T elem) const {
      return _stringer_impl<T>()(elem);
    }
    inline std::string operator()(venom_cell elem) const {
      return operator()(extractor()(elem));
    }
  };

  struct hash {
    inline int64_t operator()(T elem) const {
      return _hash_impl<T>()(elem);
    }
    inline int64_t operator()(venom_cell elem) const {
      return operator()(extractor()(elem));
    }
  };

  struct equal_to {
    inline bool operator()(T lhs, T rhs) const {
      return _equal_impl<T>()(lhs, rhs);
    }
    inline bool operator()(venom_cell lhs, venom_cell rhs) const {
      extractor x;
      return operator()(x(lhs), x(rhs));
    }
  };
};

}
}

#endif /* VENOM_RUNTIME_CELLUTILS_H */
