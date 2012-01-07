#ifndef VENOM_AST_WHILESTMT_H
#define VENOM_AST_WHILESTMT_H

#include <iostream>

#include <ast/expression/node.h>
#include <ast/statement/node.h>

#include <util/stl.h>
#include <util/macros.h>

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

  virtual size_t getNumKids() const { return 2; }

  virtual ASTNode* getNthKid(size_t kid) {
    ASTNode *kids[] = {cond, stmts};
    VENOM_SAFE_RETURN(kids, kid);
  }

  virtual void setNthKid(size_t idx, ASTNode* kid) {
    VENOM_CHECK_RANGE(idx, 2);
    VENOM_SAFE_SET2(cond, stmts, kid, idx);
  }

  virtual bool needsNewScope(size_t k) const {
    VENOM_CHECK_RANGE(k, 2);
    return k == 1;
  }

  virtual void codeGen(backend::CodeGenerator& cg);

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
