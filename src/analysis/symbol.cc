#include <sstream>
#include <stdexcept>

#include <ast/statement/classdecl.h>
#include <ast/statement/funcdecl.h>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>

using namespace std;
using namespace venom::ast;

namespace venom {
namespace analysis {

string
BaseSymbol::getFullName() const {
  stringstream buf;
  buf << getDefinedSymbolTable()->getSemanticContext()->getFullModuleName();
  buf << "." << getName();
  return buf.str();
}

bool
BaseSymbol::isModuleLevelSymbol() const {
  return getDefinedSymbolTable()->isModuleLevelSymbolTable();
}

bool
BaseSymbol::isLocalTo(const SymbolTable* query) const {
  assert(query);
  if (table == query) return true;
  // grab owner of query
  const ASTNode* owner = query->getOwner();
  if (!owner ||
      // TODO: i think checking for passing just a FuncDeclNode alone
      // is sufficient...
      dynamic_cast<const ClassDeclNode*>(owner) ||
      dynamic_cast<const FuncDeclNode*>(owner)) return false;
  return isLocalTo(owner->getSymbolTable());
}

InstantiatedType*
Symbol::bind(SemanticContext* ctx, TypeTranslator& t,
             const InstantiatedTypeVec& params) {
  if (!type) return NULL;
  InstantiatedType* translated = t.translate(ctx, type);
  return isPromoteToRef() ?
    Type::RefType->instantiate(ctx, util::vec1(translated)) :
    translated;
}

void
SlotMixin::checkCanGetIndex() {
  BaseSymbol* bs = getThisSymbol();
  VENOM_ASSERT_NOT_NULL(bs);

  // 3 cases
  // A) module level symbol
  // B) object field
  // C) method

  if (Symbol* sym = dynamic_cast<Symbol*>(bs)) {
    if (sym->isModuleLevelSymbol()) return;
    if (sym->isObjectField()) return;
  } else if (dynamic_cast<MethodSymbol*>(bs)) {
    return;
  }

  throw runtime_error(
      "Cannot ask for field index on non-slot symbol: " +
      VENOM_SOURCE_INFO);
}

size_t
SlotMixin::getFieldIndexImpl() {
  checkCanGetIndex();
  if (fieldIndex == -1) {
    // need to compute

    // get class symbol for module
    ClassSymbol *csym = getClassSymbolForSlotCalc();
    VENOM_ASSERT_NOT_NULL(csym);

    vector<Symbol*> attributes;
    vector<FuncSymbol*> methods;
    csym->linearizedOrder(attributes, methods);

    for (size_t i = 0; i < attributes.size(); i++) {
      Symbol* s = attributes[i];
      assert(s->fieldIndex == -1 || s->fieldIndex >= 0);
      // indicies should never change once they are set...
      assert(s->fieldIndex == -1 || size_t(s->fieldIndex) == i);
      s->fieldIndex = i;
    }

    for (size_t i = 0; i < methods.size(); i++) {
      FuncSymbol* fs = methods[i];
      if (MethodSymbol* ms = dynamic_cast<MethodSymbol*>(fs)) {
        assert(ms->fieldIndex == -1 || ms->fieldIndex >= 0);
        // indicies should never change once they are set...
        assert(ms->fieldIndex == -1 || size_t(ms->fieldIndex) == i);
        ms->fieldIndex = i;
      }
    }
  }
  assert(fieldIndex >= 0);
  return size_t(fieldIndex);
}

ClassSymbol*
Symbol::getClassSymbolForSlotCalc() {
  assert(isModuleLevelSymbol());
  SemanticContext* mainCtx =
    getDefinedSymbolTable()->getSemanticContext();
  ModuleSymbol* msym =
    mainCtx->getRootSymbolTable()->findModuleSymbol(
      mainCtx->getModuleName(), SymbolTable::NoRecurse);
  VENOM_ASSERT_NOT_NULL(msym);
  return msym->getModuleClassSymbol();
}

void
ClassAttributeSymbol::cloneForTemplate(
    ClassSymbol* newParent, const TypeTranslator& t) {
  SymbolTable* table = newParent->getClassSymbolTable();
  SemanticContext* ctx = table->getSemanticContext();
  table->createClassAttributeSymbol(
      name, t.translate(ctx, getInstantiatedType()), newParent);
}

InstantiatedType*
FuncSymbol::bind(SemanticContext* ctx, TypeTranslator& t,
                 const InstantiatedTypeVec& params) {
  if (typeParams.size() != params.size()) {
    throw TypeViolationException(
        "Expected " + util::stringify(typeParams.size()) +
        " type arguments to function " + name + ", got " +
        util::stringify(params.size()));
  }

  // add mapping to the type translator
  TypeMap map(typeParams.size());
  util::zip(typeParams.begin(), typeParams.end(),
            params.begin(), map.begin());
  t.map.insert(t.map.end(), map.begin(), map.end());

  // TODO: bind params to func type when we have parameterized
  // function types
  if (this->params.size() >= Type::FuncTypes.size()) {
    // TODO: better error message
    throw runtime_error("Too many parameters");
  }

  vector<InstantiatedType*> fparams(this->params);
  fparams.push_back(returnType);
  InstantiatedType *ret =
    Type::FuncTypes.at(this->params.size())->instantiate(ctx, fparams);
  assert(ret);
  return t.translate(ctx, ret);
}

string
MethodSymbol::getFullName() const {
  stringstream buf;
  buf << getClassSymbol()->getFullName();
  buf << "." << getName();
  return buf.str();
}

void
MethodSymbol::cloneForTemplate(
    ClassSymbol* newParent, const TypeTranslator& t) {
  SymbolTable* table = newParent->getClassSymbolTable();
  SemanticContext* ctx = table->getSemanticContext();
  TypeTranslator tt;
  table->createMethodSymbol(
      name,
      getTypeParams(),
      util::transform_vec(getParams().begin(), getParams().end(),
        TypeTranslator::TranslateFunctor(ctx, t)),
      t.translate(ctx, getReturnType()),
      newParent,
      table->findFuncSymbol(name, SymbolTable::ClassParents, tt),
      isNative());
}

InstantiatedType*
ClassSymbol::bind(SemanticContext* ctx, TypeTranslator& t,
                  const InstantiatedTypeVec& params) {
  return t.translate(
      ctx,
      Type::ClassType->instantiate(
        ctx, util::vec1(type->instantiate(ctx, params))));
}

InstantiatedType*
ClassSymbol::getSelfType(SemanticContext* ctx) {
  return type->instantiate(ctx, typeParams);
}

bool
ClassSymbol::isTopLevelClass() const {
  const ASTNode *owner = getDefinedSymbolTable()->getOwner();
  return !owner || (!owner->getEnclosingFuncNode() &&
                    !owner->getEnclosingClassNode());
}

bool
ClassSymbol::isModuleClassSymbol() const {
  return type->getParams() == 0 &&
    type->instantiate()
        ->isSubtypeOf(*InstantiatedType::ModuleType);
}

void
ClassSymbol::linearizedOrder(vector<Symbol*>& attributes,
                             vector<FuncSymbol*>& methods) {
  if (isModuleClassSymbol()) {
    // special case module symbols
    // only return members as attributes
    classTable->getSymbols(attributes);
    return;
  }

  vector<SymbolTable*> tables;
  classTable->linearizedClassOrder(tables);

  // attributes is easy, since there isn't any overriding
  // of attributes, we simply concat the attributes in order
  for (vector<SymbolTable*>::iterator it = tables.begin();
       it != tables.end(); ++it) {
    (*it)->getSymbols(attributes);
  }

  // methods are trickier, b/c they can override
  map<MethodSymbol*, size_t> index;
  for (vector<SymbolTable*>::iterator it = tables.begin();
       it != tables.end(); ++it) {
    vector<FuncSymbol*> clsMethods;
    (*it)->getFuncSymbols(clsMethods);
    methods.reserve(methods.size() + clsMethods.size());
    for (vector<FuncSymbol*>::iterator fit = clsMethods.begin();
         fit != clsMethods.end(); ++fit) {
      if (MethodSymbol *msym = dynamic_cast<MethodSymbol*>((*fit))) {
        if (msym->isConstructor()) {
          // don't include constructors
          continue;
        }
        if (!msym->getOverrides()) {
          map<MethodSymbol*, size_t>::iterator it =
            index.find(msym);
          assert(it == index.end());
          index[msym] = methods.size();
          methods.push_back(msym);
        } else {
          VENOM_ASSERT_TYPEOF_PTR(MethodSymbol, msym->getOverrides());
          // find the symbol being overridden, and replace it
          map<MethodSymbol*, size_t>::iterator it =
            index.find(static_cast<MethodSymbol*>(msym->getOverrides()));
          assert(it != index.end());
          methods[it->second] = msym;
          index[msym] = it->second;
        }
      } else {
        methods.push_back((*fit));
      }
    }
  }
}

ClassSymbol*
ClassSymbol::instantiateSpecializedType(const TypeTranslator& t) {
  assert(getClassSymbolTable()->getOwner() == NULL);
  assert(!getTypeParams().empty());
  assert(getType()->hasParams());

  SemanticContext* ctx = getDefinedSymbolTable()->getSemanticContext();

  // concrete type
  InstantiatedType* concreteType = t.translate(ctx, getSelfType(ctx));
  InstantiatedType::AssertNoTypeParamPlaceholders(concreteType);

  // do parents first
  InstantiatedType* parentType =
    t.translate(ctx, getType()->getParent());
  InstantiatedType::AssertNoTypeParamPlaceholders(parentType);

  ClassSymbol* csym = NULL;
  if (!parentType->getParams().empty()) {
    // first, look up the type to see if we already have it
    TypeTranslator tt;
    csym =
      parentType->getClassSymbol()->getDefinedSymbolTable()->findClassSymbol(
          parentType->createClassName(), SymbolTable::NoRecurse, tt);
    if (!csym) {
      TypeTranslator t;
      t.bind(parentType);
      csym = parentType->getClassSymbol()->instantiateSpecializedType(t);
    }
  } else csym = parentType->getClassSymbol();
  assert(csym);

  Type* type = ctx->createType(
      concreteType->createClassName(), parentType, 0);
  SymbolTable* newTable = getDefinedSymbolTable()->newChildScope(NULL);
  ClassSymbol* newSym = getDefinedSymbolTable()->createSpecializedClassSymbol(
      newTable, concreteType, type);

  // clone all attributes + methods
  vector<Symbol*> attrs;
  getClassSymbolTable()->getSymbols(attrs);

  vector<FuncSymbol*> methods;
  getClassSymbolTable()->getFuncSymbols(methods);

  vector<BaseSymbol*> syms;
  syms.reserve(attrs.size() + methods.size());
  syms.insert(syms.end(), attrs.begin(), attrs.end());
  syms.insert(syms.end(), methods.begin(), methods.end());

  for (vector<BaseSymbol*>::iterator it = syms.begin();
       it != syms.end(); ++it) {
    (*it)->cloneForTemplate(newSym, t);
  }
  return newSym;
}

InstantiatedType*
ModuleSymbol::bind(SemanticContext* ctx, TypeTranslator& t,
                   const InstantiatedTypeVec& params) {
  if (origCtx != ctx) {
    throw TypeViolationException(
        "Cannot access imported modules of another module");
  }
  return t.translate(
      ctx, moduleClassSymbol->getType()->instantiate(ctx, params));
}

}
}
