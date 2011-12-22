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

  virtual size_t getNumKids() const { return values.size(); }

  virtual ASTNode* getNthKid(size_t kid) { return values.at(kid); }

  virtual bool needsNewScope(size_t k) const { return false; }

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(arrayliteral ";
    PrintExprNodeVec(o, values, indent);
    o << ")";
  }

private:
  ExprNodeVec values;
};

}
}

#endif /* VENOM_AST_ARRAYLITERAL_H */
