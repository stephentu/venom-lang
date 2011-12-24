#include <cassert>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>

#include <ast/expression/variable.h>
#include <ast/statement/assign.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

void
AssignNode::RegisterSymbolForAssignment(SemanticContext* ctx,
                                        SymbolTable*     symbols,
                                        VariableNode*    var) {
  // check for duplicate definition
  if (symbols->isDefined(var->getName(), SymbolTable::Any, false)) {
    throw SemanticViolationException(
        "Symbol " + var->getName() + " already defined");
  }

  InstantiatedType *itype;
  if (var->getExplicitParameterizedTypeString()) {
    itype = ctx->instantiateOrThrow(
        symbols, var->getExplicitParameterizedTypeString());
  } else {
    itype = InstantiatedType::AnyType;
  }

  symbols->createSymbol(var->getName(), itype);
}

void
AssignNode::registerSymbol(SemanticContext* ctx) {
  VariableNode *vn = dynamic_cast<VariableNode*>(variable);
  if (vn) RegisterSymbolForAssignment(ctx, symbols, vn);
}

void
AssignNode::semanticCheckImpl(SemanticContext* ctx, bool doRegister) {
  // Do the right child first (this prevents recursive assignment, ie x = x)
  if (value) value->semanticCheckImpl(ctx, true);
  if (doRegister) {
    registerSymbol(ctx);
  }
  // dont recurse on variable...
}

}
}
