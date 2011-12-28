#ifndef VENOM_AST_RETURN_H
#define VENOM_AST_RETURN_H

#include <ast/expression/node.h>
#include <ast/statement/node.h>

#include <util/macros.h>

namespace venom {
namespace ast {

class ReturnNode : public ASTStatementNode {
public:
  /** Takes ownership of expr */
  ReturnNode(ASTExpressionNode* expr)
    : expr(expr) {}

  ~ReturnNode() {
    if (expr) delete expr;
  }

  virtual size_t getNumKids() const { return 1; }

  virtual ASTNode* getNthKid(size_t kid) {
    ASTNode *kids[] = {expr};
    VENOM_SAFE_RETURN(kids, kid);
  }

  virtual bool needsNewScope(size_t k) const { return false; }

  virtual void registerSymbol(analysis::SemanticContext* ctx);

  virtual void typeCheck(analysis::SemanticContext* ctx,
                         analysis::InstantiatedType* expected = NULL);

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(return ";
    if (expr) expr->print(o, indent);
    o << ")";
  }

private:
  ASTExpressionNode* expr;
};

}
}

#endif /* VENOM_AST_RETURN_H */
