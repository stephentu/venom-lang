#ifndef VENOM_AST_NILLITERAL_H
#define VENOM_AST_NILLITERAL_H

#include <stdexcept>

#include <ast/node.h>

namespace venom {
namespace ast {

class NilLiteralNode : public ASTExpressionNode {
public:
  NilLiteralNode() {}

  virtual size_t getNumKids() const { return 0; }

  virtual ASTNode* getNthKid(size_t kid) {
    throw std::out_of_range(__func__);
  }

  virtual bool needsNewScope(size_t k) const { return false; }

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(nillit)";
  }
};

}
}

#endif /* VENOM_AST_NILLITERAL_H */
