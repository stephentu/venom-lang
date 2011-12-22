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

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(assign ";
    variable->print(o, indent);
    o << " ";
    value->print(o, indent);
    o << ")";
  }

private:
  ASTExpressionNode* variable;
  ASTExpressionNode* value;
};

}
}

#endif /* VENOM_AST_ASSIGN_H */
