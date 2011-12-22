#ifndef VENOM_AST_WHILESTMT_H
#define VENOM_AST_WHILESTMT_H

#include <ast/expression/node.h>
#include <ast/statement/node.h>

namespace venom {
namespace ast {

class WhileStmtNode : public ASTStatementNode {
public:
  /** Takes ownership of cond and stmts */
  WhileStmtNode(ASTExpressionNode* cond,
                ASTStatementNode*  stmts)
    : cond(cond), stmts(stmts) {}

  ~WhileStmtNode() {
    delete cond;
    delete stmts;
  }

private:
  ASTExpressionNode* cond;
  ASTStatementNode*  stmts;
};

}
}

#endif /* VENOM_AST_WHILESTMT_H */
