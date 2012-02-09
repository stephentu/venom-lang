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

#include <algorithm>
#include <cassert>

#include <analysis/semanticcontext.h>
#include <analysis/symboltable.h>

#include <ast/expression/node.h>
#include <ast/statement/classdecl.h>

using namespace std;
using namespace venom::ast;

namespace venom {
namespace analysis {

SymbolTable::SymbolTable(SemanticContext* ctx, SymbolTable* primaryParent,
                         ASTNode* owner, ASTNode* node)
  : ctx(ctx),
    owner(owner),
    node(node),
    primaryParent(primaryParent),
    symbolContainer(
        primaryParent ?
          util::vec1(&primaryParent->symbolContainer) :
          CType<Symbol*>::vec_type(),
        util::vec1(TypeMap())),
    funcContainer(
        primaryParent ?
          util::vec1(&primaryParent->funcContainer) :
          CType<FuncSymbol*>::vec_type(),
        util::vec1(TypeMap())),
    classContainer(
        primaryParent ?
          util::vec1(&primaryParent->classContainer) :
          CType<ClassSymbol*>::vec_type(),
        util::vec1(TypeMap())),
    moduleContainer(
        primaryParent ?
          util::vec1(&primaryParent->moduleContainer) :
          CType<ModuleSymbol*>::vec_type(),
        util::vec1(TypeMap())) {
}

bool
SymbolTable::belongsTo(const SymbolTable* parent) const {
  const SymbolTable* cur = this;
  while (cur) {
    if (cur == parent) return true;
    cur = cur->getPrimaryParent();
  }
  return false;
}

bool
SymbolTable::canSee(BaseSymbol* bs) {
  // TODO: not the greatest implementation...
  TypeTranslator t;
  if (Symbol *sym = dynamic_cast<Symbol*>(bs)) {
    Symbol *ret;
    return symbolContainer.find(ret, sym->getName(), AllowCurrentScope, t,
                                equality_find_filter<Symbol*>(sym));
  } else if (FuncSymbol *fs = dynamic_cast<FuncSymbol*>(bs)) {
    FuncSymbol *ret;
    return funcContainer.find(ret, fs->getName(), AllowCurrentScope, t,
                              equality_find_filter<FuncSymbol*>(fs));
  } else if (ClassSymbol *cs = dynamic_cast<ClassSymbol*>(bs)) {
    ClassSymbol *ret;
    return classContainer.find(ret, cs->getName(), AllowCurrentScope, t,
                               equality_find_filter<ClassSymbol*>(cs));
  } else if (ModuleSymbol *ms = dynamic_cast<ModuleSymbol*>(bs)) {
    ModuleSymbol *ret;
    return moduleContainer.find(ret, ms->getName(), AllowCurrentScope, t,
                                equality_find_filter<ModuleSymbol*>(ms));
  } else assert(false);
  return false;
}

bool
SymbolTable::isModuleLevelSymbolTable() const {
  assert(ctx);
  return ctx->getModuleRoot() ?
    this == ctx->getModuleRoot()->getSymbolTable() :
    // TODO: if there's no moduleroot, then its a builtin
    // symbol, and we need another way of identifying
    // if its a module level builtin symbol
    false;
}

bool
SymbolTable::isClassSymbolTable() const {
  return dynamic_cast<const ClassDeclNode*>(getOwner());
}

bool
SymbolTable::isDefined(const string& name,
                       unsigned int type,
                       RecurseMode mode) {
  TypeTranslator t;
  return findBaseSymbol(name, type, mode, t);
}

size_t
SymbolTable::countClassBoundaries(const SymbolTable* outer) const {
  const SymbolTable* cur = this;
  bool foundOuter = false;
  size_t n = 0;
  while (cur) {
    if (cur == outer) {
      foundOuter = true;
      break;
    }
    if (cur->isClassSymbolTable()) n++;
    cur = cur->getPrimaryParent();
  }
  if (!foundOuter) {
    throw runtime_error("outer is not ancestor of this table");
  }
  return n;
}

BaseSymbol*
SymbolTable::findBaseSymbol(const string& name,
                            unsigned int type,
                            RecurseMode mode,
                            TypeTranslator& translator) {
  // TODO: this is actually really broken in the case where
  // multiple symbols of different types are defined. we should really
  // be searching all containers simultaneously level by level.
  assert(type & (Location | Function | Class | Module));
  BaseSymbol *ret = NULL;
  if ((type & Location) && (ret = findSymbol(name, mode, translator)))
    return ret;
  if ((type & Function) && (ret = findFuncSymbol(name, mode, translator)))
    return ret;
  if ((type & Class) && (ret = findClassSymbol(name, mode, translator)))
    return ret;
  if ((type & Module) && (ret = findModuleSymbol(name, mode)))
    return ret;
  return NULL;
}

Symbol*
SymbolTable::createSymbol(const string&     name,
                          InstantiatedType* type,
                          ASTNode*          decl) {
  Symbol *sym = new Symbol(name, this, type, decl);
  symbolContainer.insert(name, sym);
  return sym;
}

Symbol*
SymbolTable::createClassAttributeSymbol(
  const string& name, InstantiatedType* type, ClassSymbol* classSymbol) {

  Symbol *sym = new ClassAttributeSymbol(name, this, type, classSymbol);
  symbolContainer.insert(name, sym);
  return sym;
}

Symbol*
SymbolTable::findSymbol(const string& name, RecurseMode mode,
                        TypeTranslator& translator) {
  Symbol *ret = NULL;
  symbolContainer.find(ret, name, mode, translator);
  return ret;
}

void
SymbolTable::getSymbols(vector<Symbol*>& symbols) {
  symbolContainer.getAll(symbols);
}

FuncSymbol*
SymbolTable::createFuncSymbol(const string&                    name,
                              SymbolTable*                     funcTable,
                              const vector<InstantiatedType*>& typeParams,
                              const vector<InstantiatedType*>& params,
                              InstantiatedType*                returnType,
                              bool                             native) {
  FuncSymbol *sym = new FuncSymbol(
      name, typeParams, this, funcTable, params, returnType, native);
  funcContainer.insert(name, sym);
  return sym;
}

FuncSymbol*
SymbolTable::createMethodSymbol(const string&                    name,
                                SymbolTable*                     funcTable,
                                const vector<InstantiatedType*>& typeParams,
                                const vector<InstantiatedType*>& params,
                                InstantiatedType*                returnType,
                                ClassSymbol*                     classSymbol,
                                bool                             overrides,
                                bool                             native) {
  FuncSymbol *sym = new MethodSymbol(
      name, typeParams, this, funcTable, params,
      returnType, native, classSymbol, overrides);
  funcContainer.insert(name, sym);
  return sym;
}

FuncSymbol*
SymbolTable::findFuncSymbol(const string& name, RecurseMode mode,
                            TypeTranslator& translator) {
  FuncSymbol *ret = NULL;
  funcContainer.find(ret, name, mode, translator);
  return ret;
}

void
SymbolTable::getFuncSymbols(vector<FuncSymbol*>& symbols) {
  funcContainer.getAll(symbols);
}

ClassSymbol*
SymbolTable::createClassSymbol(const string& name,
                               SymbolTable*  classTable,
                               Type*         type,
                               const vector<InstantiatedType*>& typeParams) {
  assert(type->getParams() == typeParams.size());
  assert(name == type->getName());
  return insertClassSymbol(
      new ClassSymbol(name, typeParams, this, classTable, type));
}

ClassSymbol*
SymbolTable::createSpecializedClassSymbol(
    SymbolTable* classTable, InstantiatedType* instantiation, Type* type) {
  assert(type->getParams() == 0);
  InstantiatedType::AssertNoTypeParamPlaceholders(instantiation);
  assert(instantiation->createClassName() == type->getName());
  return insertClassSymbol(
      new SpecializedClassSymbol(instantiation, this, classTable, type));
}

ClassSymbol*
SymbolTable::insertClassSymbol(ClassSymbol* sym) {
  Type* type = sym->getType();
  type->setClassSymbol(sym);
  classContainer.insert(sym->getName(), sym);

  // link the stmts symbol table to the parents symbol tables
  if (type->getParent()) {
    TypeTranslator t;
    t.bind(type->getParent());
    sym->getClassSymbolTable()->addClassParent(
        type->getParent(), t.map);
  }
  return sym;
}

ClassSymbol*
SymbolTable::findClassSymbol(const string& name, RecurseMode mode,
                             TypeTranslator& translator) {
  ClassSymbol *ret = NULL;
  classContainer.find(ret, name, mode, translator);
  return ret;
}

void
SymbolTable::getClassSymbols(vector<ClassSymbol*>& symbols) {
  classContainer.getAll(symbols);
}

ModuleSymbol*
SymbolTable::findModuleSymbol(const string& name, RecurseMode mode) {
  ModuleSymbol *ret = NULL;
  TypeTranslator t;
  moduleContainer.find(ret, name, mode, t);
  return ret;
}

ModuleSymbol*
SymbolTable::createModuleSymbol(const string& name, SymbolTable* moduleTable,
                                ClassSymbol* moduleClass, SemanticContext* origCtx) {
  ModuleSymbol *sym =
    new ModuleSymbol(name, this, moduleTable, moduleClass, origCtx);
  moduleContainer.insert(name, sym);
  return sym;
}

void
SymbolTable::getModuleSymbols(vector<ModuleSymbol*>& symbols) {
  moduleContainer.getAll(symbols);
}

void
SymbolTable::linearizedClassOrder(vector<SymbolTable*>& tables) {
  for (vector<InstantiatedType*>::iterator it = classParents.begin();
       it != classParents.end(); ++it) {
    ClassSymbol *csym = (*it)->findCodeGeneratableClassSymbol();
    csym->getClassSymbolTable()->linearizedClassOrder(tables);
  }
  tables.push_back(this);
}

}
}
