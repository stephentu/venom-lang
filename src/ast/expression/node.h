#ifndef VENOM_AST_EXPRESSION_NODE_H
#define VENOM_AST_EXPRESSION_NODE_H

#include <iostream>
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

inline void PrintExprNodeVec(std::ostream& o,
                             const ExprNodeVec& exprs,
                             size_t indent) {
  o << "(";
  for (ExprNodeVec::const_iterator it = exprs.begin();
       it != exprs.end(); ++it) {
    (*it)->print(o, indent);
    if (it + 1 != exprs.end()) o << " ";
  }
  o << ")";
}

}
}

#endif /* VENOM_AST_EXPRESSION_NODE_H */
