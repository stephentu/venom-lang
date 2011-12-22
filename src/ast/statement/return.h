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
private:
  ASTExpressionNode* expr;
};

}
}

#endif /* VENOM_AST_RETURN_H */
