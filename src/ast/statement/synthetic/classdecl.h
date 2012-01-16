#ifndef VENOM_AST_SYNTHETIC_CLASSDECL_H
#define VENOM_AST_SYNTHETIC_CLASSDECL_H

#include <ast/statement/classdecl.h>

namespace venom {
namespace ast {

class ClassDeclNodeSynthetic : public ClassDeclNode {
public:
  ClassDeclNodeSynthetic(
      const std::string& name,
      const std::vector<analysis::InstantiatedType*>& parentTypes,
      const std::vector<analysis::InstantiatedType*>& typeParamTypes,
      ASTStatementNode* stmts)
    : ClassDeclNode(name, stmts),
      parentTypes(parentTypes), typeParamTypes(typeParamTypes) {
    assert(!parentTypes.empty());
    // TODO: implementation limitation
    assert(parentTypes.size() == 1);
  }

  virtual std::vector<analysis::InstantiatedType*> getParents() const
    { return parentTypes; }

  virtual std::vector<analysis::InstantiatedType*> getTypeParams() const
    { return typeParamTypes; }

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL(ClassDeclNode)

  virtual void print(std::ostream& o, size_t indent = 0);

protected:
  virtual void checkAndInitTypeParams(analysis::SemanticContext* ctx);
  virtual void checkAndInitParents(analysis::SemanticContext* ctx);

private:
  std::vector<analysis::InstantiatedType*> parentTypes;
  std::vector<analysis::InstantiatedType*> typeParamTypes;
};

}
}

#endif /* VENOM_AST_SYNTHETIC_CLASSDECL_H */
