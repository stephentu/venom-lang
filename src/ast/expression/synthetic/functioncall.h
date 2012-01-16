#ifndef VENOM_AST_SYNTHETIC_FUNCTIONCALL_H
#define VENOM_AST_SYNTHETIC_FUNCTIONCALL_H

#include <ast/expression/functioncall.h>

namespace venom {
namespace ast {

class FunctionCallNodeSynthetic : public FunctionCallNode {
public:
  FunctionCallNodeSynthetic(
      ASTExpressionNode* primary,
      const std::vector<analysis::InstantiatedType*>& typeArgTypes,
      const ExprNodeVec& args)
    : FunctionCallNode(primary, args), typeArgTypes(typeArgTypes) {}

  virtual std::vector<analysis::InstantiatedType*> getTypeParams() const
    { return typeArgTypes; }

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL(FunctionCallNode)

protected:
  virtual void checkAndInitTypeParams(analysis::SemanticContext* ctx) {}

private:
  std::vector<analysis::InstantiatedType*> typeArgTypes;
};

}
}

#endif /* VENOM_AST_SYNTHETIC_FUNCTIONCALL_H */
