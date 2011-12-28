#include <analysis/type.h>
#include <analysis/semanticcontext.h>

#include <ast/expression/node.h>
#include <ast/statement/node.h>

#include <util/macros.h>

using namespace venom::analysis;

namespace venom {
namespace ast {

void ASTStatementNode::checkExpectedType(InstantiatedType* expected) {
  if (expected && !expected->equals(*InstantiatedType::VoidType)) {
    throw TypeViolationException(
        "Expected type " + expected->stringify() + ", got void type");
  }
}

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

}
}
