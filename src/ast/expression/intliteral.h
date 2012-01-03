#ifndef VENOM_AST_INTLITERAL_H
#define VENOM_AST_INTLITERAL_H

#include <stdexcept>

#include <ast/expression/node.h>

namespace venom {
namespace ast {

class IntLiteralNode : public ASTExpressionNode {
public:
  IntLiteralNode(int64_t value) : value(value) {}

  virtual size_t getNumKids() const { return 0; }

  virtual ASTNode* getNthKid(size_t kid) {
    throw std::out_of_range(__PRETTY_FUNCTION__);
  }

  virtual bool needsNewScope(size_t k) const { return false; }

protected:
  virtual analysis::InstantiatedType*
    typeCheckImpl(analysis::SemanticContext* ctx,
                  analysis::InstantiatedType* expected,
                  const analysis::InstantiatedTypeVec& typeParamArgs);

public:
  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(intlit " << value << ")";
  }

private:
  int64_t value;
};

}
}

#endif /* VENOM_AST_INTLITERAL_H */
