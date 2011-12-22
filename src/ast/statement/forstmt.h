#ifndef VENOM_AST_FORSTMT_H
#define VENOM_AST_FORSTMT_H

#include <ast/expression/node.h>
#include <ast/statement/node.h>

namespace venom {
namespace ast {

class ForStmtNode : public ASTStatementNode {
public:
  /** Takes ownership of variable, expr, stmts */
  ForStmtNode(ASTExpressionNode* variable,
              ASTExpressionNode* expr,
              ASTStatementNode*  stmts)
    : variable(variable), expr(expr), stmts(stmts) {}

  ~ForStmtNode() {
    delete variable;
    delete expr;
    delete stmts;
  }

private:
  ASTExpressionNode* variable;
  ASTExpressionNode* expr;
  ASTStatementNode*  stmts;
};

}
}

#endif /* VENOM_AST_FORSTMT_H */
