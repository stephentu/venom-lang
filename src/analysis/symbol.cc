#include <stdexcept>

#include <ast/statement/classdecl.h>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>

using namespace std;
using namespace venom::ast;

namespace venom {
namespace analysis {

InstantiatedType*
Symbol::bind(SemanticContext* ctx, TypeTranslator& t,
             const InstantiatedTypeVec& params) {
  return t.translate(ctx, type);
}

bool FuncSymbol::isConstructor() const {
  const ClassDeclNode *cdn =
    dynamic_cast<const ClassDeclNode*>(getDefinedSymbolTable()->getOwner());
  if (!cdn) return false;
  return (cdn->getName() == name);
}

bool FuncSymbol::isMethod() const {
  const ClassDeclNode *cdn =
    dynamic_cast<const ClassDeclNode*>(getDefinedSymbolTable()->getOwner());
  return cdn;
}

InstantiatedType*
FuncSymbol::bind(SemanticContext* ctx, TypeTranslator& t,
                 const InstantiatedTypeVec& params) {
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

InstantiatedType*
ClassSymbol::bind(SemanticContext* ctx, TypeTranslator& t,
                  const InstantiatedTypeVec& params) {
  return t.translate(ctx, type->instantiate(ctx, params));
}

}
}
