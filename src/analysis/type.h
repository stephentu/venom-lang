/**
 * Copyright (c) 2012 Stephen Tu
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names
 * of its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef VENOM_ANALYSIS_TYPE_H
#define VENOM_ANALYSIS_TYPE_H

#include <cassert>
#include <stdexcept>
#include <string>
#include <vector>
#include <utility>

#include <util/stl.h>

namespace venom {

namespace ast {
  /** Forward decl */
  class ASTExpressionNode;
  class ParameterizedTypeString;
}

namespace analysis {

/** Forward decl */
class ClassSymbol;
class InstantiatedType;
class MethodSymbol;
class SemanticContext;
class SymbolTable;
class TypeTranslator;

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

  SymbolTable* getClassSymbolTable();
  const SymbolTable* getClassSymbolTable() const;

  inline InstantiatedType* getParent() { return parent; }
  inline const InstantiatedType* getParent() const { return parent; }

  inline size_t getParams() const { return params; }
  inline bool hasParams() const { return params > 0; }

  /** Is this type only visible to the scope it is defined in
   * (and not child scopes)? */
  virtual bool isCurrentScopeOnly() const { return false; }

  virtual std::string stringify() const;

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

  bool isListType() const;
  bool isMapType() const;
  bool isRefType() const;

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
    bool eqSyms = symbol == other.symbol;
    assert(!eqSyms || (name == other.name));
    return eqSyms;
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
  inline size_t getPos() const { return pos; }

  virtual bool isCurrentScopeOnly() const { return true; }

  // TODO: not really sure if equals() is necessary...
  virtual bool equals(const Type& other) const;
  virtual std::string stringify() const;

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
    assert(IsFullyInstantiated(types.begin(), types.end()));
  }

  template <typename ForwardIterator>
  static bool IsFullyInstantiated(ForwardIterator begin, ForwardIterator end) {
    while (begin != end) {
      if (!(*begin)->isFullyInstantiated()) return false;
      ++begin;
    }
    return true;
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

  InstantiatedType* getParentInstantiatedType();

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
  inline bool isVoid() const { return getType()->isVoid(); }
  inline bool isAny() const { return getType()->isAny(); }

  inline bool isNumeric() const { return getType()->isNumeric(); }
  inline bool isPrimitive() const { return getType()->isPrimitive(); }
  inline bool isRefCounted() const { return getType()->isRefCounted(); }
  inline bool isFunction() const { return getType()->isFunction(); }
  inline bool isClassType() const { return getType()->isClassType(); }
  inline bool isModuleType() const { return getType()->isModuleType(); }

  /** Is this type visible to the program (can we assign a reference to it?) */
  inline bool isVisible() const { return !isModuleType(); }

  inline SymbolTable* getClassSymbolTable()
    { return getType()->getClassSymbolTable(); }
  inline const SymbolTable* getClassSymbolTable() const
    { return getType()->getClassSymbolTable(); }

  /** this =:= other? */
  bool equals(const InstantiatedType& other) const;

  /** this <: other ? */
  bool isSubtypeOf(const InstantiatedType& other);

  /** Find the most common type between this type and other */
  InstantiatedType* mostCommonType(InstantiatedType* other);

  virtual std::string stringify() const;

  struct StringerFunctor :
    public util::stringify_functor<InstantiatedType>::ptr {};

  inline std::string createClassName() const {
    return createClassNameImpl(false);
  }

  ClassSymbol* findSpecializedClassSymbol();

  ClassSymbol* findCodeGeneratableClassSymbol();

  InstantiatedType* findCodeGeneratableIType(analysis::SemanticContext* ctx);

  InstantiatedType* refify(analysis::SemanticContext* ctx);

  /**
   * Returns true if this type is a specialization of that
   */
  bool isSpecializationOf(InstantiatedType* that) const;

  /**
   * If findOrigDef is true, finds the original
   * definition for the method symbol. If it is false,
   * finds the first occurance from this type all the
   * way up to type Any */
  inline MethodSymbol*
  findMethodSymbol(const std::string& name,
                   InstantiatedType*& klass,
                   bool findOrigDef = false) {
    MethodSymbol* ms = NULL; klass = NULL;
    findMethodSymbolImpl(name, ms, klass, findOrigDef);
    assert(bool(ms) == bool(klass));
    return ms;
  }

  ast::ParameterizedTypeString*
    toParameterizedString(SymbolTable* boundaryScope);

  struct ToParameterizedStringFunctor {
    ToParameterizedStringFunctor(SymbolTable* boundaryScope)
      : boundaryScope(boundaryScope) {}
    typedef ast::ParameterizedTypeString* result_type;
    inline ast::ParameterizedTypeString* operator()(InstantiatedType* t) const
      { return t->toParameterizedString(boundaryScope); }
  private:
    analysis::SymbolTable* boundaryScope;
  };

protected:
  void
  findMethodSymbolImpl(const std::string& name,
                       MethodSymbol*& ms,
                       InstantiatedType*& klass,
                       bool findOrigDef = false);

private:
  struct class_name_functor {
    inline std::string operator()(const InstantiatedType* t) const {
      return t->createClassNameImpl(true);
    }
  };

  std::string createClassNameImpl(bool fullName) const;

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
