#include <cassert>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>

#include <ast/expression/variable.h>
#include <ast/statement/assign.h>
#include <ast/statement/classattrdecl.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

void
ClassAttrDeclNode::registerSymbol(SemanticContext* ctx) {
  VariableNode *var = dynamic_cast<VariableNode*>(variable);
  assert(var);

  // don't allow an attr to overshadow any decl
  // in a parent
  if (symbols->isDefined(
        var->getName(), SymbolTable::Any, SymbolTable::ClassParents)) {
    throw SemanticViolationException(
        "Name " + var->getName() + " already defined in parent");
  }

  if (symbols->isDefined(
        var->getName(), SymbolTable::Any, SymbolTable::NoRecurse)) {
    throw SemanticViolationException(
        "Name " + var->getName() + " already defined in class");
  }

  InstantiatedType *itype = NULL;
  if (var->getExplicitParameterizedTypeString()) {
    itype = ctx->instantiateOrThrow(
        symbols, var->getExplicitParameterizedTypeString());
  }

  symbols->createSymbol(var->getName(), itype);
}

void
ClassAttrDeclNode::semanticCheckImpl(SemanticContext* ctx, bool doRegister) {
  // Do the right child first (this prevents recursive assignment, ie x = x)
  if (value) value->semanticCheckImpl(ctx, true);
  if (doRegister) {
    registerSymbol(ctx);
  }
  // dont recurse on variable...
}

void
ClassAttrDeclNode::typeCheck(SemanticContext* ctx,
                             InstantiatedType* expected) {
  assert(!expected);
  if (value) AssignNode::TypeCheckAssignment(ctx, symbols, variable, value);
}

}
}
