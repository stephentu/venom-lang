#ifndef VENOM_AST_IFSTMT_H
#define VENOM_AST_IFSTMT_H

#include <iostream>

#include <ast/expression/node.h>
#include <ast/statement/node.h>

#include <util/stl.h>
#include <util/macros.h>

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

  virtual size_t getNumKids() const { return 3; }

  virtual ASTNode* getNthKid(size_t kid) {
    ASTNode *kids[] = {cond, true_branch, false_branch};
    VENOM_SAFE_RETURN(kids, kid);
  }

  virtual void setNthKid(size_t idx, ASTNode* kid) {
    VENOM_CHECK_RANGE(idx, 3);
    VENOM_SAFE_SET3(cond, true_branch, false_branch, kid, idx);
  }

  virtual bool needsNewScope(size_t k) const {
    VENOM_CHECK_RANGE(k, 3);
    return k != 0;
  }

  virtual void typeCheck(analysis::SemanticContext* ctx,
                         analysis::InstantiatedType* expected = NULL);

  virtual void codeGen(backend::CodeGenerator& cg);

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL(IfStmtNode)

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
