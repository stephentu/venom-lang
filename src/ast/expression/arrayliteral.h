#ifndef VENOM_AST_ARRAYLITERAL_H
#define VENOM_AST_ARRAYLITERAL_H

#include <ast/expression/node.h>
#include <util/stl.h>

namespace venom {
namespace ast {

class ArrayLiteralNode : public ASTExpressionNode {
public:

  /** Takes ownership of the nodes in values */
  ArrayLiteralNode(const ExprNodeVec& values)
    : values(values) {}

  ~ArrayLiteralNode() {
    util::delete_pointers(values.begin(), values.end());
  }
private:
  ExprNodeVec values;
};

}
}

#endif /* VENOM_AST_ARRAYLITERAL_H */
