#ifndef VENOM_AST_INTLITERAL_H
#define VENOM_AST_INTLITERAL_H

#include <ast/node.h>

namespace venom {
namespace ast {

class IntLiteralNode : public ASTExpressionNode {
public:
  IntLiteralNode(int64_t value) : value(value) {}
private:
  int64_t value;
};

}
}

#endif /* VENOM_AST_INTLITERAL_H */
