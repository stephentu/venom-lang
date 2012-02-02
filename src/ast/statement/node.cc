#include <cassert>

#include <analysis/type.h>
#include <analysis/semanticcontext.h>

#include <ast/expression/node.h>
#include <ast/statement/node.h>

#include <util/macros.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

void ASTStatementNode::typeCheck(SemanticContext* ctx,
                                 InstantiatedType* expected) {
  forchild (kid) {
    if (kid) {
      if (ASTStatementNode *stmt = dynamic_cast<ASTStatementNode*>(kid)) {
        stmt->typeCheck(ctx);
      } else if (ASTExpressionNode *expr = dynamic_cast<ASTExpressionNode*>(kid)) {
        expr->typeCheck(ctx);
      }
    }
  } endfor
  checkExpectedType(expected);
}

void
ASTStatementNode::liftPhaseImpl(SemanticContext* ctx,
                                SymbolTable* liftInto,
                                vector<ASTStatementNode*>& liftedStmts) {
  forchild (kid) {
    if (!kid || kid->isTypeParameterized()) continue;
    if (ASTStatementNode *stmt = dynamic_cast<ASTStatementNode*>(kid)) {
      stmt->liftPhaseImpl(ctx, liftInto, liftedStmts);
    }
  } endfor
}

ASTStatementNode*
ASTStatementNode::replace(SemanticContext* ctx, ASTNode* replacement) {
  assert(replacement);
  assert(dynamic_cast<ASTStatementNode*>(replacement));
  return static_cast<ASTStatementNode*>(ASTNode::replace(ctx, replacement));
}

void ASTStatementNode::checkExpectedType(InstantiatedType* expected) {
  if (expected && !expected->equals(*InstantiatedType::VoidType)) {
    throw TypeViolationException(
        "Expected type " + expected->stringify() + ", got void type");
  }
}

}
}
