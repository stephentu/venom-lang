#ifndef VENOM_AST_NILLITERAL_H
#define VENOM_AST_NILLITERAL_H

#include <stdexcept>

#include <ast/expression/node.h>

namespace venom {
namespace ast {

class NilLiteralNode : public ASTExpressionNode {
public:
  NilLiteralNode() {}

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
    o << "(nillit)";
  }
};

}
}

#endif /* VENOM_AST_NILLITERAL_H */
