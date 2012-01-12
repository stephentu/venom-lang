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

void
ClassSymbol::linearizedOrder(vector<ClassAttributeSymbol*>& attributes,
                             vector<MethodSymbol*>& methods) {
  vector<SymbolTable*> tables;
  classTable->linearizedClassOrder(tables);

  // attributes is easy, since there isn't any overriding
  // of attributes, we simply concat the attributes in order
  for (vector<SymbolTable*>::iterator it = tables.begin();
       it != tables.end(); ++it) {
    vector<Symbol*> clsAttrs;
    (*it)->getSymbols(clsAttrs);
    attributes.resize(attributes.size() + clsAttrs.size());
    transform(
        clsAttrs.begin(), clsAttrs.end(),
        attributes.begin(),
        util::poly_ptr_cast_functor<Symbol, ClassAttributeSymbol>::checked());
  }

  // methods are trickier, b/c they can override
  for (vector<SymbolTable*>::iterator it = tables.begin();
       it != tables.end(); ++it) {
    vector<FuncSymbol*> clsMethods;
    (*it)->getFuncSymbols(clsMethods);
    methods.reserve(methods.size() + clsMethods.size());
    for (vector<FuncSymbol*>::iterator fit = clsMethods.begin();
         fit != clsMethods.end(); ++fit) {
      VENOM_ASSERT_TYPEOF_PTR(MethodSymbol, (*fit));
      MethodSymbol *msym = static_cast<MethodSymbol*>((*fit));
      if (!msym->getOverrides()) methods.push_back(msym);
    }
  }
}

InstantiatedType*
ModuleSymbol::bind(SemanticContext* ctx, TypeTranslator& t,
                   const InstantiatedTypeVec& params) {
  if (origCtx != ctx) {
    throw TypeViolationException(
        "Cannot access imported modules of another module");
  }
  return t.translate(ctx, moduleType->instantiate(ctx, params));
}

}
}
