#ifndef VENOM_AST_FORSTMT_H
#define VENOM_AST_FORSTMT_H

#include <iostream>

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

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(for ";
    variable->print(o, indent);
    o << " ";
    expr->print(o, indent);
    o << std::endl << util::indent(indent + 1);
    stmts->print(o, indent + 1);
    o << ")";
  }

private:
  ASTExpressionNode* variable;
  ASTExpressionNode* expr;
  ASTStatementNode*  stmts;
};

}
}

#endif /* VENOM_AST_FORSTMT_H */
