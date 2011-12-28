#include <ast/expression/variable.h>

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

}
}
