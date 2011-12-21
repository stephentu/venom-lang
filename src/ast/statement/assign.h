#ifndef VENOM_AST_ASSIGN_H
#define VENOM_AST_ASSIGN_H

#include <string>

#include <ast/expression/node.h>
#include <ast/statement/node.h>

namespace venom {
namespace ast {

class AssignNode : public ASTStatementNode {
public:
  /** Takes ownership of variable, value */
  AssignNode(ASTExpressionNode* variable, ASTExpressionNode* value)
    : variable(variable), value(value) {}

  ~AssignNode() {
    delete variable;
    delete value;
  }
private:
  ASTExpressionNode* variable;
  ASTExpressionNode* value;
};

}
}

#endif /* VENOM_AST_ASSIGN_H */
