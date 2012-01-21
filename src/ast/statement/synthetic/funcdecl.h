#ifndef VENOM_AST_SYNTHETIC_FUNCDECL_H
#define VENOM_AST_SYNTHETIC_FUNCDECL_H

#include <ast/statement/funcdecl.h>

namespace venom {
namespace ast {

class FuncDeclNodeSynthetic : public FuncDeclNode {
public:
  FuncDeclNodeSynthetic(
      const std::string& name,
      const std::vector<analysis::InstantiatedType*>& typeParamTypes,
      const ExprNodeVec& params,
      analysis::InstantiatedType* retType,
      ASTStatementNode* stmts)
    : FuncDeclNode(name, params, stmts),
      typeParamTypes(typeParamTypes),
      retType(retType) {
      assert(retType);
    }

  virtual std::vector<analysis::InstantiatedType*> getTypeParams() const
    { return typeParamTypes; }

  virtual analysis::InstantiatedType* getReturnType() const { return retType; }

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL_STMT(FuncDeclNode)

  virtual void print(std::ostream& o, size_t indent = 0);

protected:
  virtual void checkAndInitTypeParams(analysis::SemanticContext* ctx);
  virtual void checkAndInitReturnType(analysis::SemanticContext* ctx);

  std::vector<analysis::InstantiatedType*> typeParamTypes;
  analysis::InstantiatedType* retType;
};

}
}

#endif /* VENOM_AST_SYNTHETIC_FUNCDECL_H */
