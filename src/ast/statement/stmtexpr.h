#ifndef VENOM_AST_STMTEXPR_H
#define VENOM_AST_STMTEXPR_H

#include <ast/expression/node.h>
#include <ast/statement/node.h>

#include <util/macros.h>

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

  virtual size_t getNumKids() const { return 1; }

  virtual ASTNode* getNthKid(size_t kid) {
    ASTNode *kids[] = {expr};
    VENOM_SAFE_RETURN(kids, kid);
  }

  virtual void setNthKid(size_t idx, ASTNode* kid) {
    VENOM_CHECK_RANGE(idx, 1);
    VENOM_SAFE_SET_EXPR(expr, kid);
  }

  virtual bool needsNewScope(size_t k) const {
    VENOM_CHECK_RANGE(k, 1);
    return false;
  }

  virtual void typeCheck(analysis::SemanticContext* ctx,
                         analysis::InstantiatedType* expected = NULL);

  virtual void codeGen(backend::CodeGenerator& cg);

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL(StmtExprNode)

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(stmtexpr ";
    expr->print(o, indent);
    o << ")";
  }

protected:
  virtual ASTNode* rewriteReturn(analysis::SemanticContext* ctx);

private:
  ASTExpressionNode* expr;
};

}
}

#endif /* VENOM_AST_STMTEXPR_H */
