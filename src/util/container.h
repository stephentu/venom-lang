#ifndef VENOM_UTIL_CONTAINER_H
#define VENOM_UTIL_CONTAINER_H

#include <map>
#include <vector>

namespace venom {
namespace util {

template <typename SearchType, typename StoreType>
struct container_base {
  typedef std::vector<StoreType> vec_type;
  typedef std::map<SearchType, size_t> map_type;

  vec_type vec;
  map_type map;

  template <typename Functor>
  size_t createImpl(const SearchType& t, Functor f, bool& create) {
    typename map_type::iterator it = map.find(t);
    if (it == map.end()) {
      size_t ret = vec.size();
      vec.push_back(f(t));
      map[t] = ret;
      create = true;
      return ret;
    } else {
      create = false;
      return it->second;
    }
  }

  inline void reset() {
    vec.clear();
    map.clear();
  }
};

template <typename SearchType>
struct container_pool_functor {
  inline SearchType operator()(const SearchType& t) const {
    return t;
  }
};

template <typename SearchType>
struct container_pool : public container_base<SearchType, SearchType> {
  inline size_t create(const SearchType& t, bool& create) {
    return createImpl(t, container_pool_functor<SearchType>(), create);
  }
};

template <typename Obj>
class SizedArray {
public:
  typedef Obj* array_type;

  SizedArray()
    : elems(NULL), n_elems(0) {}
  SizedArray(array_type elems, size_t n_elems)
    : elems(elems), n_elems(n_elems) {}

  inline array_type begin() { return elems; }
  inline const array_type begin() const { return elems; }

  inline array_type end() { return elems + n_elems; }
  inline const array_type end() const { return elems + n_elems; }

  inline size_t size() const { return n_elems; }

private:
  array_type elems;
  size_t n_elems;
};

}
}

#endif /* VENOM_UTIL_CONTAINER_H */
