#include <ast/expression/variable.h>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>

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
VariableNode::typeCheck(SemanticContext*  ctx,
                        InstantiatedType* expected) {
  BaseSymbol *sym = symbols->findBaseSymbol(name, SymbolTable::Any, true);
  assert(sym);
  return sym->bind(ctx, vector<InstantiatedType*>());
}

}
}
