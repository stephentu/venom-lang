#include <cassert>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>

#include <ast/expression/variable.h>
#include <ast/statement/classattrdecl.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

void
ClassAttrDeclNode::registerSymbol(analysis::SemanticContext* ctx) {
  VariableNode *var = dynamic_cast<VariableNode*>(variable);
  assert(var);

  // check for duplicate definition
  if (symbols->isDefined(var->getName(), SymbolTable::Any, false)) {
    throw SemanticViolationException(
        "Field " + var->getName() + " already defined");
  }

  InstantiatedType *itype;
  if (var->getExplicitParameterizedTypeString()) {
    itype = ctx->instantiateOrThrow(
        symbols, var->getExplicitParameterizedTypeString());
  } else {
    itype =
      ctx
        ->getRootSymbolTable()
        ->findClassSymbol("any", false)
        ->getType()
        ->instantiate(ctx);
  }

  symbols->createSymbol(var->getName(), itype);
}

}
}

