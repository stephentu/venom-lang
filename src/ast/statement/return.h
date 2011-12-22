#ifndef VENOM_AST_RETURN_H
#define VENOM_AST_RETURN_H

#include <ast/expression/node.h>
#include <ast/statement/node.h>

namespace venom {
namespace ast {

class ReturnNode : public ASTStatementNode {
public:
  /** Takes ownership of expr */
  ReturnNode(ASTExpressionNode* expr)
    : expr(expr) {}

  ~ReturnNode() {
    delete expr;
  }

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(return ";
    expr->print(o, indent);
    o << ")";
  }

private:
  ASTExpressionNode* expr;
};

}
}

#endif /* VENOM_AST_RETURN_H */
