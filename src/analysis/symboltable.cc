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

struct functor {
  functor(SemanticContext* ctx, const TypeTranslator* t)
    : ctx(ctx), t(t) {}
  inline InstantiatedType* operator()(InstantiatedType* type) const {
    return t->translate(ctx, type);
  }
  SemanticContext* ctx;
  const TypeTranslator* t;
};

struct find_functor {
  find_functor(InstantiatedType* type) : type(type) {}
  inline bool operator()(const InstantiatedTypePair& p) const {
    return p.first->equals(*type);
  }
  InstantiatedType* type;
};

InstantiatedType*
TypeTranslator::translate(SemanticContext* ctx, InstantiatedType* type) const {
  TypeMap::const_iterator it =
    find_if(map.begin(), map.end(), find_functor(type));
  if (it != map.end()) return it->second;
  vector<InstantiatedType*> buf(type->getParams().size());
  transform(type->getParams().begin(), type->getParams().end(),
            buf.begin(), functor(ctx, this));
  return ctx->createInstantiatedType(type->getType(), buf);
}

void
TypeTranslator::bind(InstantiatedType* type) {
  vector<InstantiatedType*> &lhs =
    type->getType()->getClassSymbol()->getTypeParams();
  vector<InstantiatedType*> &rhs = type->getParams();
  assert(lhs.size() == rhs.size());
  TypeMap tmap(lhs.size());
  util::zip(lhs.begin(), lhs.end(), rhs.begin(), tmap.begin());
  map.insert(map.end(), tmap.begin(), tmap.end());
}

SymbolTable::SymbolTable(SemanticContext* ctx, SymbolTable* parent,
                         const TypeMap& map, ASTNode* owner)
  : ctx(ctx),
    owner(owner),
    parents(parent ? util::vec1(parent) : vector<SymbolTable*>()),
    symbolContainer(
        parent ?
          util::vec1(&parent->symbolContainer) :
          CType<Symbol*>::vec_type(),
        parent ?
          util::vec1(map) :
          vector<TypeMap>()),
    funcContainer(
        parent ?
          util::vec1(&parent->funcContainer) :
          CType<FuncSymbol*>::vec_type(),
        parent ?
          util::vec1(map) :
          vector<TypeMap>()),
    classContainer(
        parent ?
          util::vec1(&parent->classContainer) :
          CType<ClassSymbol*>::vec_type(),
        parent ?
          util::vec1(map) :
          vector<TypeMap>()),
    moduleContainer(
        parent ?
          util::vec1(&parent->moduleContainer) :
          CType<ModuleSymbol*>::vec_type(),
        parent ?
          util::vec1(map) :
          vector<TypeMap>()) {
  assert(parent || map.empty());
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
  return this == ctx->getModuleRoot()->getSymbolTable();
}

bool
SymbolTable::isDefined(const string& name,
                       unsigned int type,
                       RecurseMode mode) {
  TypeTranslator t;
  return findBaseSymbol(name, type, mode, t);
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
                          InstantiatedType* type) {
  Symbol *sym = new Symbol(name, this, type);
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
                              const vector<InstantiatedType*>& typeParams,
                              const vector<InstantiatedType*>& params,
                              InstantiatedType*                returnType,
                              bool                             native) {
  FuncSymbol *sym =
    new FuncSymbol(name, typeParams, this, params, returnType, native);
  funcContainer.insert(name, sym);
  return sym;
}

FuncSymbol*
SymbolTable::createMethodSymbol(const string&                    name,
                                const vector<InstantiatedType*>& typeParams,
                                const vector<InstantiatedType*>& params,
                                InstantiatedType*                returnType,
                                ClassSymbol*                     classSymbol,
                                FuncSymbol*                      overrides,
                                bool                             native) {
  FuncSymbol *sym =
    new MethodSymbol(name, typeParams, this, params,
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
  ClassSymbol *sym = new ClassSymbol(name, typeParams, this, classTable, type);
  type->setClassSymbol(sym);
  classContainer.insert(name, sym);
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
  for (vector<SymbolTable*>::iterator it = parents.begin() + 1;
       it != parents.end(); ++it) {
    (*it)->linearizedClassOrder(tables);
  }
  tables.push_back(this);
}

}
}
