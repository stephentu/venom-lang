#ifndef VENOM_AST_STMTEXPR_H
#define VENOM_AST_STMTEXPR_H

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

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(stmtexpr ";
    expr->print(o, indent);
    o << ")";
  }

private:
  ASTExpressionNode* expr;
};

}
}

#endif /* VENOM_AST_STMTEXPR_H */
