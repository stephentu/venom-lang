#ifndef VENOM_ANALYSIS_SYMBOL_H
#define VENOM_ANALYSIS_SYMBOL_H

#include <cassert>
#include <string>
#include <vector>

#include <analysis/type.h>

namespace venom {

namespace ast {
  /** Forward decl */
  class AssignNode;
  class FuncDeclNode;
}

namespace backend {
  /** Forward decl */
  class CodeGenerator;
}

namespace analysis {

/** Forward decl */
class ClassSymbol;
class SemanticContext;
class SymbolTable;
class TypeTranslator;

class BaseSymbol {
public:
  virtual ~BaseSymbol() {}

  inline std::string& getName() { return name; }
  inline const std::string& getName() const { return name; }

  /** Get the fully qualified name of this symbol */
  virtual std::string getFullName() const;

  inline SymbolTable* getDefinedSymbolTable() { return table; }
  inline const SymbolTable* getDefinedSymbolTable() const { return table; }

  /** Is this symbol defined in a top level module? */
  bool isModuleLevelSymbol() const;

  /**
   * Precondition: this symbol is defined in either
   * query, or a primary ancestor of query
   */
  bool isLocalTo(const SymbolTable* query) const;

  virtual InstantiatedType*
    bind(SemanticContext* ctx,
         const TypeTranslator& t,
         const InstantiatedTypeVec& params) = 0;

  /** Is this symbol visible to the current scope (and not
   * descendants) only? */
  virtual bool isCurrentScopeOnly() const { return false; }

  virtual void cloneForTemplate(
      ClassSymbol* newParent, const TypeTranslator& t) = 0;

protected:
  BaseSymbol(const std::string& name,
             SymbolTable*       table)
    : name(name), table(table) {}
  std::string  name;
  SymbolTable* table;
};

class SlotMixin {
public:
  SlotMixin() : fieldIndex(-1) {}

  size_t getFieldIndex() const
    { return const_cast<SlotMixin*>(this)->getFieldIndexImpl(); }

protected:
  size_t getFieldIndexImpl();

  virtual BaseSymbol* getThisSymbol() = 0;
  virtual ClassSymbol* getClassSymbolForSlotCalc() = 0;

  mutable ssize_t fieldIndex;

private:
  void checkCanGetIndex();
};

/**
 * Represents a location in memory, such as a variable, or a
 * object's field.
 */
class Symbol : public BaseSymbol, public SlotMixin {
  friend class ast::AssignNode;
  friend class backend::CodeGenerator;
  friend class SymbolTable;
protected:
  Symbol(const std::string& name,
         SymbolTable*       table,
         InstantiatedType*  type,
         ast::ASTNode*      decl)
    : BaseSymbol(name, table),
      promoteToRef(false),
      type(type), decl(decl) {}

  virtual BaseSymbol* getThisSymbol() { return this; }
  virtual ClassSymbol* getClassSymbolForSlotCalc();

public:

  inline bool isPromoteToRef() const { return promoteToRef; }

  inline void markPromoteToRef() {
    assert(!isObjectField());
    assert(!promoteToRef);
    assert(type);
    promoteToRef = true;
  }

  inline InstantiatedType* getInstantiatedType() { return type; }
  inline const InstantiatedType* getInstantiatedType() const { return type; }

  /** Can only do if no type set */
  inline void setInstantiatedType(InstantiatedType* type)
    { assert(!this->type); assert(type); this->type = type; }

  inline ast::ASTNode* getDecl() { return decl; }
  inline const ast::ASTNode* getDecl() const { return decl; }

  virtual InstantiatedType*
    bind(SemanticContext* ctx,
         const TypeTranslator& t,
         const InstantiatedTypeVec& params);

  virtual bool isObjectField() const { return false; }

  virtual void cloneForTemplate(
      ClassSymbol* newParent, const TypeTranslator& t) {
    VENOM_NOT_REACHED;
  }

private:
  unsigned promoteToRef : 1;
  InstantiatedType* type;
  ast::ASTNode* decl;
};

class ClassAttributeSymbol : public Symbol {
  friend class SymbolTable;
protected:
  ClassAttributeSymbol(const std::string& name,
                       SymbolTable*       table,
                       InstantiatedType*  type,
                       ClassSymbol*       classSymbol)
    : Symbol(name, table, type, NULL),
      classSymbol(classSymbol) {}

  virtual ClassSymbol* getClassSymbolForSlotCalc() { return classSymbol; }

public:
  inline ClassSymbol* getClassSymbol() { return classSymbol; }
  inline const ClassSymbol* getClassSymbol() const { return classSymbol; }

  virtual bool isObjectField() const { return true; }

  virtual void cloneForTemplate(
      ClassSymbol* newParent, const TypeTranslator& t);

private:
  ClassSymbol* classSymbol;
};

/**
 * Represents a function/method declaration
 */
class FuncSymbol : public BaseSymbol {
  friend class ast::FuncDeclNode;
  friend class SymbolTable;

protected:
  FuncSymbol(const std::string&         name,
             const InstantiatedTypeVec& typeParams,
             SymbolTable*               table,
             const InstantiatedTypeVec& params,
             InstantiatedType*          returnType,
             bool                       native)
    : BaseSymbol(name, table), typeParams(typeParams),
      params(params), returnType(returnType), native(native) {}

public:

  inline InstantiatedTypeVec&
    getTypeParams() { return typeParams; }
  inline const InstantiatedTypeVec&
    getTypeParams() const { return typeParams; }

  inline InstantiatedTypeVec&
    getParams() { return params; }
  inline const InstantiatedTypeVec&
    getParams() const { return params; }

  inline InstantiatedType*
    getReturnType() { return returnType; }
  inline const InstantiatedType*
    getReturnType() const { return returnType; }

  inline bool isNative() const { return native; }

  virtual InstantiatedType*
    bind(SemanticContext* ctx,
         const TypeTranslator& t,
         const InstantiatedTypeVec& params);

  virtual bool isConstructor() const { return false; }

  virtual bool isMethod() const { return false; }

  virtual bool isCodeGeneratable() const
    { return typeParams.empty(); }

  virtual void cloneForTemplate(
      ClassSymbol* newParent, const TypeTranslator& t)
    { VENOM_NOT_REACHED; }

private:
  InstantiatedTypeVec typeParams;
  InstantiatedTypeVec params;
  InstantiatedType*   returnType;
  bool                native;
};

class MethodSymbol : public FuncSymbol, public SlotMixin {
  friend class ast::FuncDeclNode;
  friend class SymbolTable;
protected:
  MethodSymbol(const std::string&         name,
               const InstantiatedTypeVec& typeParams,
               SymbolTable*               table,
               const InstantiatedTypeVec& params,
               InstantiatedType*          returnType,
               bool                       native,
               ClassSymbol*               classSymbol,
               bool                       overrides)
    : FuncSymbol(name, typeParams, table, params, returnType, native),
      classSymbol(classSymbol), overrides(overrides) {}

  virtual BaseSymbol* getThisSymbol() { return this; }
  virtual ClassSymbol* getClassSymbolForSlotCalc() { return classSymbol; }
public:

  virtual std::string getFullName() const;

  inline ClassSymbol* getClassSymbol() { return classSymbol; }
  inline const ClassSymbol* getClassSymbol() const { return classSymbol; }

  inline bool isOverride() const { return overrides; }

  virtual bool isConstructor() const { return name == "<ctor>"; }
  virtual bool isMethod() const { return true; }

  virtual bool isCodeGeneratable() const;

  virtual void cloneForTemplate(
      ClassSymbol* newParent, const TypeTranslator& t);

private:
  ClassSymbol* classSymbol;
  bool overrides;
};

/**
 * Represents a class declaration
 */
class ClassSymbol : public BaseSymbol {
  friend class SymbolTable;
protected:
  ClassSymbol(const std::string&         name,
              const InstantiatedTypeVec& typeParams,
              SymbolTable*               table,      /* defined */
              SymbolTable*               classTable, /* class's table */
              Type*                      type)
    : BaseSymbol(name, table), typeParams(typeParams),
      classTable(classTable), type(type) {
    assert(type->getParams() == typeParams.size());
  }

public:
  inline InstantiatedTypeVec&
    getTypeParams() { return typeParams; }
  inline const InstantiatedTypeVec&
    getTypeParams() const { return typeParams; }

  inline SymbolTable* getClassSymbolTable() { return classTable; }
  inline const SymbolTable* getClassSymbolTable() const { return classTable; }

  inline Type* getType() { return type; }
  inline const Type* getType() const { return type; }

  virtual InstantiatedType*
    bind(SemanticContext* ctx,
         const TypeTranslator& t,
         const InstantiatedTypeVec& params);

  InstantiatedType* getSelfType(SemanticContext* ctx);

  bool isTopLevelClass() const;

  virtual bool isCurrentScopeOnly() const {
    return type->isCurrentScopeOnly();
  }

  virtual bool isSpecialized() const { return false; }

  /** true if either no type params, or is a specialized class */
  inline bool isCodeGeneratable() const
    { return typeParams.empty() || isSpecialized(); }

  bool isModuleClassSymbol() const;

  /** Does *NOT* include constructor in methods */
  void linearizedOrder(std::vector<Symbol*>& attributes,
                       std::vector<FuncSymbol*>& methods);

  virtual void cloneForTemplate(
      ClassSymbol* newParent, const TypeTranslator& t)
    { VENOM_NOT_REACHED; }

  /**
   * Only works for built-in class symbols.
   * Requires the type has non-zero type params
   */
  ClassSymbol* instantiateSpecializedType(const TypeTranslator& t);

private:
  InstantiatedTypeVec typeParams;
  SymbolTable*        classTable;
  Type*               type;
};

class SpecializedClassSymbol : public ClassSymbol {
  friend class SymbolTable;
protected:
  SpecializedClassSymbol(
      InstantiatedType* instantiation,
      SymbolTable*      table,      /* defined */
      SymbolTable*      classTable, /* class's table */
      Type*             type)
  : ClassSymbol(type->getName(),
                InstantiatedTypeVec(),
                table, classTable, type), instantiation(instantiation) {
    assert(!instantiation->getParams().empty());
    InstantiatedType::AssertNoTypeParamPlaceholders(instantiation);
    assert(instantiation->createClassName() == type->getName());
  }

public:
  inline InstantiatedType* getInstantiation()
    { return instantiation; }
  inline const InstantiatedType* getInstantiation() const
    { return instantiation; }

private:
  InstantiatedType* instantiation;
};

/**
 * Represents a module
 */
class ModuleSymbol : public BaseSymbol {
  friend class SymbolTable;
protected:
  ModuleSymbol(const std::string& name,
               SymbolTable* table, /* defined */
               SymbolTable* moduleTable, /* module's root table */
               ClassSymbol* moduleClassSymbol,
               SemanticContext* origCtx /* the module which imported */)
    : BaseSymbol(name, table), moduleTable(moduleTable),
      moduleClassSymbol(moduleClassSymbol), origCtx(origCtx) {}

public:
  inline SymbolTable*
    getModuleSymbolTable() { return moduleTable; }
  inline const SymbolTable*
    getModuleSymbolTable() const { return moduleTable; }

  inline ClassSymbol* getModuleClassSymbol() { return moduleClassSymbol; }
  inline const ClassSymbol*
    getModuleClassSymbol() const { return moduleClassSymbol; }

  /** This symbol should only be visible when the current ctx equals the
   * original (importing) context. This is to prevent seeing all of an
   * imported module's imports in the scope of the imported module. */
  inline SemanticContext*
    getOriginalContext() { return origCtx; }
  inline const SemanticContext*
    getOriginalContext() const { return origCtx; }

  virtual InstantiatedType*
    bind(SemanticContext* ctx,
         const TypeTranslator& t,
         const InstantiatedTypeVec& params);

  virtual void cloneForTemplate(
      ClassSymbol* newParent, const TypeTranslator& t)
    { VENOM_NOT_REACHED; }

private:
  SymbolTable* moduleTable;
  ClassSymbol* moduleClassSymbol;
  SemanticContext* origCtx;
};

}
}

#endif /* VENOM_ANALYSIS_SYMBOL_H */
