#ifndef VENOM_AST_BOOLLITERAL_H
#define VENOM_AST_BOOLLITERAL_H

#include <stdexcept>

#include <ast/node.h>

namespace venom {
namespace ast {

class BoolLiteralNode : public ASTExpressionNode {
public:
  BoolLiteralNode(bool value) : value(value) {}

  virtual size_t getNumKids() const { return 0; }

  virtual ASTNode* getNthKid(size_t kid) {
    throw std::out_of_range(__func__);
  }

  virtual bool needsNewScope(size_t k) const { return false; }

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(boollit " << value << ")";
  }

private:
  bool value;
};

}
}

#endif /* VENOM_AST_BOOLLITERAL_H */
