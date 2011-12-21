#ifndef VENOM_AST_STMTEXPR_H
#define VENOM_AST_STMTEXPR_H

#include <string>

#include <ast/expression/node.h>
#include <ast/statement/node.h>

namespace venom {
namespace ast {

class StmtExprNode : public ASTStatementNode {
public:
  /** Takes ownership of expr */
  StmtExprNode(ASTExpressionNode* expr)
    : expr(expr) {}

  ~StmtExprNode() {
    delete expr;
  }
private:
  ASTExpressionNode* expr;
};

}
}

#endif /* VENOM_AST_STMTEXPR_H */
