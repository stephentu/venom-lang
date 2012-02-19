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

#ifndef VENOM_ANALYSIS_SYMBOL_H
#define VENOM_ANALYSIS_SYMBOL_H

#include <cassert>
#include <string>
#include <vector>

#include <analysis/type.h>
#include <util/macros.h>

namespace venom {

namespace ast {
  /** Forward decl */
  class ASTNode;
  class AssignNode;
  class FuncDeclNode;
  class StmtListNode;
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
                       ClassSymbol*       classSymbol,
                       bool               privateVariable)
    : Symbol(name, table, type, NULL),
      classSymbol(classSymbol), privateVariable(privateVariable) {}

  virtual ClassSymbol* getClassSymbolForSlotCalc() { return classSymbol; }

public:
  inline ClassSymbol* getClassSymbol() { return classSymbol; }
  inline const ClassSymbol* getClassSymbol() const { return classSymbol; }

  inline bool isPrivateVariable() const { return privateVariable; }

  virtual bool isObjectField() const { return true; }

  virtual void cloneForTemplate(
      ClassSymbol* newParent, const TypeTranslator& t);

private:
  ClassSymbol* classSymbol;
  bool privateVariable;
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
             SymbolTable*               funcTable,
             const InstantiatedTypeVec& params,
             InstantiatedType*          returnType,
             bool                       native)
    : BaseSymbol(name, table), funcTable(funcTable), typeParams(typeParams),
      params(params), returnType(returnType), native(native) {}

public:

  inline SymbolTable* getFunctionSymbolTable()
    { return funcTable; }
  inline const SymbolTable* getFunctionSymbolTable() const
    { return funcTable; }

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
  SymbolTable*        funcTable;
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
               SymbolTable*               funcTable,
               const InstantiatedTypeVec& params,
               InstantiatedType*          returnType,
               bool                       native,
               ClassSymbol*               classSymbol,
               bool                       overrides)
    : FuncSymbol(name, typeParams, table, funcTable,
                params, returnType, native),
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
  friend class ast::StmtListNode;
  friend class SymbolTable;
protected:
  ClassSymbol(const std::string&         name,
              const InstantiatedTypeVec& typeParams,
              SymbolTable*               table,      /* defined */
              SymbolTable*               classTable, /* class's table */
              Type*                      type)
    : BaseSymbol(name, table), typeParams(typeParams),
      classTable(classTable), type(type), lifted(NULL), lifter(NULL) {
    assert(type->getParams() == typeParams.size());
  }

  /**
   * Can only set when lifted == NULL.
   * Also class must not have type params
   */
  inline void setLifted(ClassSymbol* lifted) {
    assert(!this->lifted);
    assert(typeParams.empty());
    this->lifted = lifted;
    assert(!this->lifted->lifter);
    this->lifted->lifter = this;
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

  inline ClassSymbol* getLifted() { return lifted; }
  inline const ClassSymbol* getLifted() const { return lifted; }

  virtual InstantiatedType*
    bind(SemanticContext* ctx,
         const TypeTranslator& t,
         const InstantiatedTypeVec& params);

  InstantiatedType* getSelfType(SemanticContext* ctx);

  ClassSymbol* followLiftedChain();

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

  inline ClassSymbol* getUnliftedSymbol() { return lifter; }
  inline const ClassSymbol* getUnliftedSymbol() const { return lifter; }

  inline ClassSymbol* getOriginalUnliftedSymbol() {
    ClassSymbol* cur = this;
    while (cur->lifter) cur = cur->lifter;
    return cur;
  }
  inline const ClassSymbol* getOriginalUnliftedSymbol() const
    { return const_cast<ClassSymbol*>(this)->getOriginalUnliftedSymbol(); }

  inline bool isLiftedClass() const { return lifter; }

  inline bool isLiftOf(const ClassSymbol* that) const {
    const ClassSymbol* cur = this;
    while (cur) {
      if (cur == that) return true;
      cur = cur->lifter;
    }
    return false;
  }

  bool hasOuterReference() const;

private:
  InstantiatedTypeVec typeParams;
  SymbolTable*        classTable;
  Type*               type;
  ClassSymbol*        lifted; // points to the *lifted* version of this symbol
  ClassSymbol*        lifter; // points to the original version of this *lifted* symbol
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
