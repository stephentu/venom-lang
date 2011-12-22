#ifndef VENOM_AST_IFSTMT_H
#define VENOM_AST_IFSTMT_H

#include <iostream>

#include <ast/expression/node.h>
#include <ast/statement/node.h>

#include <util/stl.h>

namespace venom {
namespace ast {

class IfStmtNode : public ASTStatementNode {
public:
  /** Takes ownership of cond, true_branch, false_branch */
  IfStmtNode(ASTExpressionNode* cond,
             ASTStatementNode*  true_branch,
             ASTStatementNode*  false_branch)
    : cond(cond), true_branch(true_branch), false_branch(false_branch) {}

  ~IfStmtNode() {
    delete cond;
    delete true_branch;
    delete false_branch;
  }

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(if ";
    cond->print(o, indent);
    o << std::endl << util::indent(indent + 1);
    true_branch->print(o, indent + 1);
    o << std::endl << util::indent(indent + 1);
    false_branch->print(o, indent + 1);
    o << ")";
  }

private:
  ASTExpressionNode* cond;
  ASTStatementNode*  true_branch;
  ASTStatementNode*  false_branch;
};

}
}

#endif /* VENOM_AST_IFSTMT_H */
