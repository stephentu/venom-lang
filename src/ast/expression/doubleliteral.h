#ifndef VENOM_AST_DOUBLELITERAL_H
#define VENOM_AST_DOUBLELITERAL_H

#include <ast/expression/node.h>

namespace venom {
namespace ast {

class DoubleLiteralNode : public ASTExpressionNode {
public:
  DoubleLiteralNode(double value) : value(value) {}
private:
  double value;
};

}
}

#endif /* VENOM_AST_DOUBLELITERAL_H */
