#ifndef VENOM_AST_EXPRESSION_NODE_H
#define VENOM_AST_EXPRESSION_NODE_H

#include <vector>

#include <ast/node.h>

namespace venom {
namespace ast {

class ASTExpressionNode : public ASTNode {
public:
  ASTExpressionNode() {}
};

typedef std::vector<ASTExpressionNode *> ExprNodeVec;

inline ExprNodeVec * MakeExprVec1(ASTExpressionNode *a0) {
  ExprNodeVec *v = new ExprNodeVec;
  v->push_back(a0);
  return v;
}

}
}

#endif /* VENOM_AST_EXPRESSION_NODE_H */
