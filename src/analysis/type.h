#ifndef VENOM_ANALYSIS_TYPE_H
#define VENOM_ANALYSIS_TYPE_H

#include <cassert>
#include <stdexcept>
#include <string>
#include <vector>
#include <utility>

namespace venom {

namespace ast {
  /** Forward decl */
  class ASTExpressionNode;
}

namespace analysis {

/** Forward decl */
class ClassSymbol;
class InstantiatedType;
class SemanticContext;
class SymbolTable;

/**
 * Represents a type in the language. Is not an instantiated type
 * (see InstantiatedType for instantiations of Type)
 */
class Type {
  friend class ClassSymbol;
  friend class InstantiatedType;
  friend class SemanticContext;
  friend class SymbolTable;

protected:
  /** Does NOT take ownership of symbol nor parent **/
  Type(const std::string& name,
       ClassSymbol*       symbol,
       InstantiatedType*  parent,
       size_t             params)
    : name(name), symbol(symbol), parent(parent),
      params(params), itype(NULL) {}

  void setClassSymbol(ClassSymbol* symbol);

  /** TODO: this is a hack for now */
  static void ResetBuiltinTypes();

public:
  virtual ~Type();

  /** Built-in types **/
  static Type* const AnyType;
  static Type* const IntType;
  static Type* const BoolType;
  static Type* const FloatType;
  static Type* const StringType;
  static Type* const VoidType;

  static Type* const Func0Type;
  static Type* const Func1Type;
  static Type* const Func2Type;
  static Type* const Func3Type;
  static Type* const Func4Type;
  static Type* const Func5Type;
  static Type* const Func6Type;
  static Type* const Func7Type;
  static Type* const Func8Type;
  static Type* const Func9Type;

  static Type* const Func10Type;
  static Type* const Func11Type;
  static Type* const Func12Type;
  static Type* const Func13Type;
  static Type* const Func14Type;
  static Type* const Func15Type;
  static Type* const Func16Type;
  static Type* const Func17Type;
  static Type* const Func18Type;
  static Type* const Func19Type;

  static Type* const ObjectType;

  /** Boxed primitive types */
  static Type* const BoxedIntType;
  static Type* const BoxedBoolType;
  static Type* const BoxedFloatType;

  /** Ref type */
  static Type* const RefType;

  /** Special type which represents a class type */
  static Type* const ClassType;

  /** Special type which represents a module type */
  static Type* const ModuleType;

  /** Special type which is the subtype of EVERY type */
  static Type* const BoundlessType;

  /** Special types which have syntactic sugar constructs
   * built into the grammar */
  static Type* const ListType;
  static Type* const MapType;

  static const std::vector<Type*> FuncTypes;

  /** Accessors **/

  inline std::string& getName() { return name; }
  inline const std::string& getName() const { return name; }

  inline ClassSymbol* getClassSymbol() { return symbol; }
  inline const ClassSymbol* getClassSymbol() const { return symbol; }

  inline InstantiatedType* getParent() { return parent; }
  inline const InstantiatedType* getParent() const { return parent; }

  inline size_t getParams() const { return params; }
  inline bool hasParams() const { return params > 0; }

  /** Is this type only visible to the scope it is defined in
   * (and not child scopes)? */
  virtual bool isCurrentScopeOnly() const { return false; }

  // TODO: stringify parameterized types
  virtual std::string stringify() const { return name; }

  virtual std::string stringifyTypename() const { return name; }

  // TODO: make these inline
  bool isInt() const;
  bool isFloat() const;
  bool isString() const;
  bool isBool() const;
  bool isVoid() const;
  bool isAny() const;

  inline bool isNumeric() const { return isInt() || isFloat(); }
  inline bool isPrimitive() const { return isNumeric() || isBool(); }
  inline bool isRefCounted() const { return !isPrimitive() && !isVoid(); }

  bool isFunction() const;
  bool isClassType() const;
  bool isModuleType() const;

  /** Is this type visible to the program (can we assign a reference to it?) */
  inline bool isVisible() const { return !isModuleType(); }

  /**
   * this =:= other?
   *
   * Note that this <: other is NOT well-defined on a Type
   * (see InstantiatedType) for subtype relations.
   */
  virtual bool equals(const Type& other) const {
    // TODO: symbol equality is implemented as pointer equality
    // for now, fix later. Also will need to move this into
    // type.cc if we don't use pointer equality (and remove the inline)
    return name == other.name && symbol == other.symbol;
  }

  /**
   * Any is the root of the type hierarchy (including primitives)
   */
  inline bool isAnyType() const {
    assert(parent || name == "any" || name == "boundless");
    return (parent == NULL) ? name == "any" : false;
  }

  inline bool isBoundlessType() const {
    assert(parent || name == "any" || name == "boundless");
    return (parent == NULL) ? name == "boundless" : false;
  }

  ast::ASTExpressionNode* createDefaultInitializer() const;

  /**
   * The input SemanticContext is the current context, which
   * is possibly different from the context that this type
   * was created in.
   */
  InstantiatedType* instantiate(SemanticContext* ctx);

  InstantiatedType*
  instantiate(SemanticContext* ctx,
              const std::vector<InstantiatedType*>& params);

protected:
  InstantiatedType* instantiate();

private:
  /** Regular (not fully qualified) */
  std::string       name;

  ClassSymbol*      symbol;

  /** Parent type of this type. Null if the root type (Any), or the lower type
   * (Boundless).
   *
   *  TODO: When we support multiple inheritance, will need to
   *  change this to a vector of types */
  InstantiatedType* parent;

  /** Number of parameterized types,
   *  ie if this is T<K, V>, then params = 2 */
  size_t            params;

  /** InstantiatedType of this type, only created if params = 0.
   * Takes ownership */
  InstantiatedType* itype;
};

/**
 * Represents a placeholder type parameter
 */
class TypeParamType : public Type {
  friend class SemanticContext;
protected:
  TypeParamType(const std::string& name, size_t pos);

public:
  virtual bool isCurrentScopeOnly() const { return true; }

  // TODO: not really sure if equals() is necessary...
  virtual bool equals(const Type& other) const;
  virtual std::string stringify() const;
  virtual std::string stringifyTypename() const;

private:
  size_t pos;
};

/**
 * Represents an instantiation of a Type
 *
 * For example, if we have some type map<k, v>, then
 * an instantiated type is something like map<int, string>
 */
class InstantiatedType {
  friend class SemanticContext;
  friend class Type;
protected:
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

public:

  static inline void AssertNoTypeParamPlaceholders(
      const InstantiatedType* type) {
    assert(type->isFullyInstantiated());
  }

  static inline void AssertNoTypeParamPlaceholders(
      const std::vector<InstantiatedType*>& types) {
#ifndef NDEBUG
    for (std::vector<InstantiatedType*>::const_iterator it = types.begin();
         it != types.end(); ++it) {
      AssertNoTypeParamPlaceholders(*it);
    }
#endif /* NDEBUG */
  }

  /** Returns true iff contains no type parameters */
  bool isFullyInstantiated() const;

  inline bool isSpecializedType() const {
    return !getParams().empty() && isFullyInstantiated();
  }

  /** Instantiations of built-in types **/
  static InstantiatedType* const AnyType;
  static InstantiatedType* const IntType;
  static InstantiatedType* const BoolType;
  static InstantiatedType* const FloatType;
  static InstantiatedType* const StringType;
  static InstantiatedType* const VoidType;
  static InstantiatedType* const ObjectType;
  static InstantiatedType* const BoxedIntType;
  static InstantiatedType* const BoxedBoolType;
  static InstantiatedType* const BoxedFloatType;
  static InstantiatedType* const ModuleType;
  static InstantiatedType* const BoundlessType;

  inline Type* getType() { return type; }
  inline const Type* getType() const { return type; }

  inline std::vector<InstantiatedType*>& getParams()
    { return params; }
  inline const std::vector<InstantiatedType*>& getParams() const
    { return params; }

  inline ClassSymbol* getClassSymbol()
    { return getType()->getClassSymbol(); }
  inline const ClassSymbol* getClassSymbol() const
    { return getType()->getClassSymbol(); }

  inline bool isInt() const { return getType()->isInt(); }
  inline bool isFloat() const { return getType()->isFloat(); }
  inline bool isString() const { return getType()->isString(); }
  inline bool isBool() const { return getType()->isBool(); }
  inline bool isAny() const { return getType()->isAny(); }

  inline bool isNumeric() const { return getType()->isNumeric(); }
  inline bool isPrimitive() const { return getType()->isPrimitive(); }
  inline bool isRefCounted() const { return getType()->isRefCounted(); }
  inline bool isFunction() const { return getType()->isFunction(); }
  inline bool isClassType() const { return getType()->isClassType(); }
  inline bool isModuleType() const { return getType()->isModuleType(); }

  /** Is this type visible to the program (can we assign a reference to it?) */
  inline bool isVisible() const { return !isModuleType(); }

  SymbolTable* getClassSymbolTable();
  const SymbolTable* getClassSymbolTable() const;

  /** this =:= other? */
  bool equals(const InstantiatedType& other) const;

  /** this <: other ? */
  bool isSubtypeOf(const InstantiatedType& other) const;

  /** Find the most common type between this type and other */
  InstantiatedType* mostCommonType(InstantiatedType* other);

  std::string stringify() const;

  std::string createClassName() const;

private:
  /** The pure type being instantiated */
  Type*                          type;

  /** The param arguments to the type */
  std::vector<InstantiatedType*> params;

};

typedef std::vector<InstantiatedType*> InstantiatedTypeVec;
typedef std::pair<InstantiatedType*, InstantiatedType*> InstantiatedTypePair;

}
}

#endif /* VENOM_ANALYSIS_TYPE_H */
