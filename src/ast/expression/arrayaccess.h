#ifndef VENOM_AST_ARRAYACCESS_H
#define VENOM_AST_ARRAYACCESS_H

#include <string>

#include <ast/expression/node.h>

namespace venom {
namespace ast {

class ArrayAccessNode : public ASTExpressionNode {
public:
  /** Takes ownership of primary and index */
  ArrayAccessNode(ASTExpressionNode* primary, ASTExpressionNode* index)
    : primary(primary), index(index) {}

  ~ArrayAccessNode() {
    delete primary;
    delete index;
  }
private:
  ASTExpressionNode* primary;
  ASTExpressionNode* index;
};

}
}

#endif /* VENOM_AST_ARRAYACCESS_H */
