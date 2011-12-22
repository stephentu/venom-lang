#ifndef VENOM_AST_WHILESTMT_H
#define VENOM_AST_WHILESTMT_H

#include <iostream>

#include <ast/expression/node.h>
#include <ast/statement/node.h>

#include <util/stl.h>

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

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(while ";
    cond->print(o, indent);
    o << std::endl << util::indent(indent + 1);
    stmts->print(o, indent + 1);
    o << ")";
  }

private:
  ASTExpressionNode* cond;
  ASTStatementNode*  stmts;
};

}
}

#endif /* VENOM_AST_WHILESTMT_H */
