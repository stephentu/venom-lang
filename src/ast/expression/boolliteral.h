#ifndef VENOM_AST_BOOLLITERAL_H
#define VENOM_AST_BOOLLITERAL_H

#include <stdexcept>

#include <ast/expression/node.h>

namespace venom {
namespace ast {

class BoolLiteralNode : public ASTExpressionNode {
public:
  BoolLiteralNode(bool value) : value(value) {}

  virtual size_t getNumKids() const { return 0; }

  virtual ASTNode* getNthKid(size_t kid) {
    throw std::out_of_range(VENOM_SOURCE_INFO);
  }

  virtual void setNthKid(size_t idx, ASTNode* kid) {
    throw std::out_of_range(VENOM_SOURCE_INFO);
  }

  virtual bool needsNewScope(size_t k) const {
    throw std::out_of_range(VENOM_SOURCE_INFO);
  }

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL_EXPR(BoolLiteralNode)

protected:
  virtual analysis::InstantiatedType*
    typeCheckImpl(analysis::SemanticContext* ctx,
                  analysis::InstantiatedType* expected,
                  const analysis::InstantiatedTypeVec& typeParamArgs);

public:
  virtual void codeGen(backend::CodeGenerator& cg);

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(boollit " << value << ")";
  }

private:
  bool value;
};

}
}

#endif /* VENOM_AST_BOOLLITERAL_H */
