#ifndef VENOM_AST_DOUBLELITERAL_H
#define VENOM_AST_DOUBLELITERAL_H

#include <ast/expression/node.h>

namespace venom {
namespace ast {

class DoubleLiteralNode : public ASTExpressionNode {
public:
  DoubleLiteralNode(double value) : value(value) {}

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(doublelit " << value << ")";
  }

private:
  double value;
};

}
}

#endif /* VENOM_AST_DOUBLELITERAL_H */
