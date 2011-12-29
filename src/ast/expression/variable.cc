#include <ast/expression/variable.h>
#include <ast/statement/classdecl.h>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

void
VariableNode::registerSymbol(SemanticContext* ctx) {
  if (!symbols->isDefined(name, SymbolTable::Any, true)) {
    throw SemanticViolationException(
        "Symbol " + name + " is not defined in scope");
  }
}

InstantiatedType*
VariableNode::typeCheck(SemanticContext* ctx,
                        InstantiatedType* expected,
                        const InstantiatedTypeVec& typeParamArgs) {
  TypeTranslator t;
  BaseSymbol *sym = symbols->findBaseSymbol(name, SymbolTable::Any, true, t);
  assert(sym);
  return sym->bind(ctx, t, typeParamArgs);
}

void
VariableSelfNode::registerSymbol(SemanticContext* ctx) {
  ClassDeclNode *cdn = getParentClassNode();
  if (!cdn) {
    throw SemanticViolationException(
        "self cannot be used outside of class scope");
  }
}

InstantiatedType*
VariableSelfNode::typeCheck(SemanticContext* ctx,
                            InstantiatedType* expected,
                            const InstantiatedTypeVec& typeParamArgs) {
  ClassDeclNode *cdn = getParentClassNode();
  assert(cdn);
  TypeTranslator t;
  ClassSymbol *cs =
    cdn->getSymbolTable()->findClassSymbol(cdn->getName(), false, t);
  assert(cs);
  return cs->getSelfType(ctx);

}

}
}
