#ifndef VENOM_AST_STRINGLITERAL_H
#define VENOM_AST_STRINGLITERAL_H

#include <stdexcept>
#include <string>

#include <ast/expression/node.h>

namespace venom {
namespace ast {

class StringLiteralNode : public ASTExpressionNode {
public:
  StringLiteralNode(const std::string& value) : value(value) {}

  virtual size_t getNumKids() const { return 0; }

  virtual ASTNode* getNthKid(size_t kid) {
    throw std::out_of_range(__PRETTY_FUNCTION__);
  }

  virtual void setNthKid(size_t idx, ASTNode* kid) {
    throw std::out_of_range(__PRETTY_FUNCTION__);
  }

  virtual bool needsNewScope(size_t k) const {
    throw std::out_of_range(__PRETTY_FUNCTION__);
  }

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL(StringLiteralNode)

protected:
  virtual analysis::InstantiatedType*
    typeCheckImpl(analysis::SemanticContext* ctx,
                  analysis::InstantiatedType* expected,
                  const analysis::InstantiatedTypeVec& typeParamArgs);

public:
  virtual void codeGen(backend::CodeGenerator& cg);

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(stringlit '" << value << "')";
  }

private:
  std::string value;
};

}
}

#endif /* VENOM_AST_STRINGLITERAL_H */
