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
  static Type* AnyType;
  static Type* IntType;
  static Type* BoolType;
  static Type* FloatType;
  static Type* StringType;
  static Type* VoidType;

  static Type* Func0Type;
  static Type* Func1Type;
  static Type* Func2Type;
  static Type* Func3Type;
  static Type* Func4Type;
  static Type* Func5Type;
  static Type* Func6Type;
  static Type* Func7Type;
  static Type* Func8Type;
  static Type* Func9Type;

  static Type* Func10Type;
  static Type* Func11Type;
  static Type* Func12Type;
  static Type* Func13Type;
  static Type* Func14Type;
  static Type* Func15Type;
  static Type* Func16Type;
  static Type* Func17Type;
  static Type* Func18Type;
  static Type* Func19Type;

  static Type* ObjectType;

  /** Boxed primitive types */
  static Type* BoxedIntType;
  static Type* BoxedBoolType;
  static Type* BoxedFloatType;

  /** Special type which represents a class type */
  static Type* ClassType;

  /** Special type which represents a module type */
  static Type* ModuleType;

  /** Special type which is the subtype of EVERY type */
  static Type* BoundlessType;

  /** Special types which have syntactic sugar constructs
   * built into the grammar */
  static Type* ListType;
  static Type* MapType;

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

  bool isInt() const;
  bool isFloat() const;
  bool isString() const;
  bool isBool() const;

  inline bool isNumeric() const { return isInt() || isFloat(); }
  inline bool isPrimitive() const { return isNumeric() || isBool(); }
  inline bool isRefCounted() const { return !isPrimitive(); }

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

private:

  InstantiatedType* instantiate();

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

  /** Instantiations of built-in types **/
  static InstantiatedType* AnyType;
  static InstantiatedType* IntType;
  static InstantiatedType* BoolType;
  static InstantiatedType* FloatType;
  static InstantiatedType* StringType;
  static InstantiatedType* VoidType;
  static InstantiatedType* ObjectType;
  static InstantiatedType* BoxedIntType;
  static InstantiatedType* BoxedBoolType;
  static InstantiatedType* BoxedFloatType;
  static InstantiatedType* ModuleType;
  static InstantiatedType* BoundlessType;

  inline Type* getType() { return type; }
  inline const Type* getType() const { return type; }

  inline std::vector<InstantiatedType*>& getParams() { return params; }
  inline const std::vector<InstantiatedType*>&
    getParams() const { return params; }

  inline bool isInt() const { return getType()->isInt(); }
  inline bool isFloat() const { return getType()->isFloat(); }
  inline bool isString() const { return getType()->isString(); }
  inline bool isBool() const { return getType()->isBool(); }

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
