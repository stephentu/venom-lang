#ifndef VENOM_AST_IFSTMT_H
#define VENOM_AST_IFSTMT_H

#include <ast/expression/node.h>
#include <ast/statement/node.h>

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

private:
  ASTExpressionNode* cond;
  ASTStatementNode*  true_branch;
  ASTStatementNode*  false_branch;
};

}
}

#endif /* VENOM_AST_IFSTMT_H */
