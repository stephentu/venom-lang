#ifndef VENOM_ANALYSIS_SYMBOL_H
#define VENOM_ANALYSIS_SYMBOL_H

#include <cassert>
#include <string>
#include <vector>

#include <analysis/type.h>

namespace venom {
namespace analysis {

/** Forward decl */
class SemanticContext;
class SymbolTable;
class TypeTranslator;

class BaseSymbol {
public:
  virtual ~BaseSymbol() {}

  inline std::string& getName() { return name; }
  inline const std::string& getName() const { return name; }

  std::string getFullName() const;

  inline SymbolTable* getDefinedSymbolTable() { return table; }
  inline const SymbolTable* getDefinedSymbolTable() const { return table; }

  /** Is this symbol defined in a top level module? */
  bool isModuleLevelSymbol() const;

  virtual InstantiatedType*
    bind(SemanticContext* ctx, TypeTranslator& t,
         const InstantiatedTypeVec& params) = 0;

  /** Is this symbol visible to the current scope (and not
   * descendants) only? */
  virtual bool isCurrentScopeOnly() const { return false; }

protected:
  BaseSymbol(const std::string& name,
             SymbolTable*       table)
    : name(name), table(table) {}
  std::string  name;
  SymbolTable* table;
};

/**
 * Represents a location in memory, such as a variable, or a
 * object's field.
 */
class Symbol : public BaseSymbol {
  friend class SymbolTable;
protected:
  Symbol(const std::string& name,
         SymbolTable*       table,
         InstantiatedType*  type)
    : BaseSymbol(name, table), type(type) {}

public:
  inline InstantiatedType* getInstantiatedType() { return type; }
  inline const InstantiatedType* getInstantiatedType() const { return type; }

  virtual InstantiatedType*
    bind(SemanticContext* ctx, TypeTranslator& t,
         const InstantiatedTypeVec& params);

private:
  InstantiatedType* type;
};

/**
 * Represents a function/method declaration
 */
class FuncSymbol : public BaseSymbol {
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
    bind(SemanticContext* ctx, TypeTranslator& t,
         const InstantiatedTypeVec& params);

  bool isConstructor() const;

  bool isMethod() const;

private:
  InstantiatedTypeVec typeParams;
  InstantiatedTypeVec params;
  InstantiatedType*   returnType;
  bool                native;
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
      classTable(classTable), type(type) {}

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
    bind(SemanticContext* ctx, TypeTranslator& t,
         const InstantiatedTypeVec& params);

  InstantiatedType* getSelfType(SemanticContext* ctx);

  bool isTopLevelClass() const;

  virtual bool isCurrentScopeOnly() const {
    return type->isCurrentScopeOnly();
  }

private:
  InstantiatedTypeVec typeParams;
  SymbolTable*        classTable;
  Type*               type;
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
               Type *moduleType,
               SemanticContext* origCtx /* the module which imported */)
    : BaseSymbol(name, table), moduleTable(moduleTable),
      moduleType(moduleType), origCtx(origCtx) {}

public:
  inline SymbolTable*
    getModuleSymbolTable() { return moduleTable; }
  inline const SymbolTable*
    getModuleSymbolTable() const { return moduleTable; }

  inline Type* getModuleType() { return moduleType; }
  inline const Type* getModuleType() const { return moduleType; }

  /** This symbol should only be visible when the current ctx equals the
   * original (importing) context. This is to prevent seeing all of an
   * imported module's imports in the scope of the imported module. */
  inline SemanticContext*
    getOriginalContext() { return origCtx; }
  inline const SemanticContext*
    getOriginalContext() const { return origCtx; }

  virtual InstantiatedType*
    bind(SemanticContext* ctx, TypeTranslator& t,
         const InstantiatedTypeVec& params);

private:
  SymbolTable* moduleTable;
  Type* moduleType;
  SemanticContext* origCtx;
};

}
}

#endif /* VENOM_ANALYSIS_SYMBOL_H */
