#include <cassert>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>

#include <ast/expression/variable.h>
#include <ast/statement/forstmt.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

void
ForStmtNode::semanticCheckImpl(SemanticContext* ctx, bool doRegister) {
  assert(doRegister);
  expr->semanticCheckImpl(ctx, true);
  registerSymbol(ctx);
  stmts->semanticCheckImpl(ctx, true);
}

void
ForStmtNode::registerSymbol(SemanticContext* ctx) {
  // put variable in the stmts symbol table
  // (of unknown type)
  VariableNode *vn = dynamic_cast<VariableNode*>(variable);
  assert(vn);
  stmts->getSymbolTable()->createSymbol(vn->getName(), NULL);
}

void
ForStmtNode::typeCheck(SemanticContext* ctx, InstantiatedType* expected) {
  InstantiatedType *iterableType = expr->typeCheck(ctx);

  // for now, assume that expr must be of type list[x] or string
  // in the future, we should allow any iterable (and string will be
  // defined as an iterable[string])
  if (!iterableType->getType()->equals(*Type::ListType) &&
      !iterableType->equals(*InstantiatedType::StringType)) {
    throw TypeViolationException(
        "Expect type list or string, got " + iterableType->stringify());
  }

  VariableNode *vn = dynamic_cast<VariableNode*>(variable);
  assert(vn);
  // now the type information is available, set it
  if (iterableType->getType()->equals(*Type::ListType)) {
    stmts->getSymbolTable()->createSymbol(
        vn->getName(), iterableType->getParams().front());
  } else {
    // string type
    stmts->getSymbolTable()->createSymbol(
        vn->getName(), InstantiatedType::StringType);
  }
  stmts->typeCheck(ctx);
  checkExpectedType(expected);
}

ASTNode*
ForStmtNode::rewriteLocal(SemanticContext* ctx) {
  for (size_t i = 1; i < getNumKids(); i++) {
    ASTNode* kid = getNthKid(i);
    assert(kid);
    ASTNode* rep = kid->rewriteLocal(ctx);
    if (rep) {
      assert(rep != kid);
      setNthKid(i, rep);
      delete kid;
    }
  }
  return NULL;
}

}
}
