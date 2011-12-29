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
                                        VariableNode*    var,
                                        bool             allowLocDups) {
  // check for duplicate definition
  if (symbols->isDefined(
        var->getName(), SymbolTable::Function | SymbolTable::Class,
        SymbolTable::NoRecurse)) {
    throw SemanticViolationException(
        "Symbol " + var->getName() + " already defined");
  }

  if (symbols->isDefined(
        var->getName(), SymbolTable::Location, SymbolTable::NoRecurse)) {
    if (allowLocDups) {
      if (var->getExplicitParameterizedTypeString()) {
        throw SemanticViolationException(
            "Cannot redeclare type of symbol " + var->getName());
      }
    } else {
      throw SemanticViolationException(
        "Symbol " + var->getName() + " already defined");
    }
  } else {
    InstantiatedType *itype = NULL;
    if (var->getExplicitParameterizedTypeString()) {
      itype = ctx->instantiateOrThrow(
          symbols, var->getExplicitParameterizedTypeString());
    }

    symbols->createSymbol(var->getName(), itype);
    assert(symbols->isDefined(
          var->getName(), SymbolTable::Location, SymbolTable::NoRecurse));
  }
}

void
AssignNode::TypeCheckAssignment(SemanticContext*   ctx,
                                SymbolTable*       symbols,
                                ASTExpressionNode* variable,
                                ASTExpressionNode* value) {
  InstantiatedType *lhs = variable->typeCheck(ctx, NULL);
  InstantiatedType *rhs = value->typeCheck(ctx, lhs);
  assert(rhs);
  if (lhs) {
    // require rhs <: lhs
    if (!rhs->isSubtypeOf(*lhs)) {
      throw TypeViolationException(
          "Cannot assign type " + rhs->stringify() + " to type " + lhs->stringify());
    }
  } else {
    VariableNode *vn = dynamic_cast<VariableNode*>(variable);
    assert(vn && !vn->getExplicitParameterizedTypeString());
    symbols->createSymbol(vn->getName(), rhs);
  }
}

void
AssignNode::registerSymbol(SemanticContext* ctx) {
  VariableNode *vn = dynamic_cast<VariableNode*>(variable);
  if (vn) RegisterSymbolForAssignment(ctx, symbols, vn, true);
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

void
AssignNode::typeCheck(SemanticContext* ctx, InstantiatedType* expected) {
  assert(value);
  TypeCheckAssignment(ctx, symbols, variable, value);
  checkExpectedType(expected);
}

}
}
