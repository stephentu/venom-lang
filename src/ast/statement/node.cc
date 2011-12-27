#include <ast/expression/node.h>
#include <ast/statement/node.h>

#include <util/macros.h>

using namespace venom::analysis;

namespace venom {
namespace ast {

void ASTStatementNode::typeCheck(SemanticContext* ctx) {
  forchild (kid) {
    if (kid) {
      if (ASTStatementNode *stmt = dynamic_cast<ASTStatementNode*>(kid)) {
        stmt->typeCheck(ctx);
      } else if (ASTExpressionNode *expr = dynamic_cast<ASTExpressionNode*>(kid)) {
        expr->typeCheck(ctx, NULL);
      }
    }
  } endfor
}

}
}
