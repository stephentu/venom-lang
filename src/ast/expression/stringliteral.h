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
    throw std::out_of_range(__func__);
  }

  virtual bool needsNewScope(size_t k) const { return false; }

  virtual analysis::InstantiatedType*
    typeCheck(analysis::SemanticContext* ctx,
              analysis::InstantiatedType* expected = NULL,
              const analysis::InstantiatedTypeVec& typeParamArgs
                = analysis::InstantiatedTypeVec());

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(stringlit '" << value << "')";
  }

private:
  std::string value;
};

}
}

#endif /* VENOM_AST_STRINGLITERAL_H */
