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

}
}
