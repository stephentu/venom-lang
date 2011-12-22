#ifndef VENOM_ANALYSIS_TYPE_H
#define VENOM_ANALYSIS_TYPE_H

#include <stdexcept>
#include <string>
#include <vector>

namespace venom {
namespace analysis {

/** Forward decl */
class SemanticContext;

/**
 * Represents a type in the language. Is not an instantiated type
 * (see InstantiatedType for instantiations of Type)
 */
class Type {
  friend class SemanticContext;
protected:
  /** Does NOT take ownership of ctx nor parent **/
  Type(const std::string& name,
       SemanticContext*   ctx,
       Type*              parent,
       size_t             params)
    : name(name), ctx(ctx), parent(parent), params(params) {}

public:
  inline std::string& getName() { return name; }
  inline const std::string& getName() const { return name; }

  inline Type* getParent() { return parent; }
  inline const Type* getParent() const { return parent; }

  inline size_t getParams() const { return params; }
  inline bool hasParams() const { return params > 0; }

  /**
   * Two types are equal iff their fully-qualified names are equal
   *
   * Equivalent, this happens if their regular names are the same and
   * their semantic contexts are the same
   */
  inline bool equals(const Type& other) const {
    // TODO: context equality is implemented as pointer equality
    // for now, fix later. Also will need to move this into
    // type.cc if we don't use pointer equality (and remove the inline)
    return name == other.name && ctx == other.ctx;
  }

  /**
   * Any is the root of the type hierarchy (including primitives)
   */
  inline bool isAnyType() const { return parent == NULL; }

private:
  /** Regular (not fully qualified) */
  std::string      name;

  /** The semantic context (ie module) of this type.
   * The fully qualified name of this type is constructed
   * from the context's name + this type's name */
  SemanticContext* ctx;

  /** Parent type of this type. Null iff the root type (Any).
   *  TODO: When we support multiple inheritance, will need to
   *  change this to a vector of types */
  Type*            parent;

  /** Number of parameterized types,
   *  ie if this is T[K, V], then params = 2 */
  size_t           params;
};

/**
 * Represents an instantiation of a Type
 *
 * For example, if we have some type map[k, v], then
 * an instantiated type is something like map[int, string]
 */
class InstantiatedType {
public:
  /**
   * Requires !type->hasParams()
   */
  InstantiatedType(Type* type)
    : type(type) {

    if (type->hasParams()) {
      throw std::invalid_argument("wrong number of params");
    }
  }

  /**
   * Requires that type->getParams() == params.size()
   */
  InstantiatedType(Type* type,
                   const std::vector<InstantiatedType*>& params)
    : type(type), params(params) {

    if (type->getParams() != params.size()) {
      throw std::invalid_argument("wrong number of params");
    }
  }

  inline Type* getType() { return type; }
  inline const Type* getType() const { return type; }

private:
  /** The pure type being instantiated */
  Type*                          type;

  /** The param arguments to the type */
  std::vector<InstantiatedType*> params;

};

}
}

#endif /* VENOM_ANALYSIS_TYPE_H */
