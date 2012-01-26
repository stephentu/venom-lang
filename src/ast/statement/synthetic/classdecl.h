#ifndef VENOM_AST_SYNTHETIC_CLASSDECL_H
#define VENOM_AST_SYNTHETIC_CLASSDECL_H

#include <ast/statement/classdecl.h>

namespace venom {

/** forward decl */
namespace analysis { class Type; }

namespace ast {

class ClassDeclNodeSynthetic : public ClassDeclNode {
public:
  ClassDeclNodeSynthetic(
      const std::string& name,
      const std::vector<analysis::InstantiatedType*>& parentTypes,
      const std::vector<analysis::InstantiatedType*>& typeParamTypes,
      ASTStatementNode* stmts,
      analysis::InstantiatedType* instantiation = NULL)
  : ClassDeclNode(name, stmts),
    parentTypes(parentTypes), typeParamTypes(typeParamTypes),
    instantiation(instantiation) {

    assert(!parentTypes.empty());
    // TODO: implementation limitation
    assert(parentTypes.size() == 1);

    assert(!instantiation || typeParamTypes.empty());
  }

  virtual std::vector<analysis::InstantiatedType*> getParents() const
    { return parentTypes; }

  virtual std::vector<analysis::InstantiatedType*> getTypeParams() const
    { return typeParamTypes; }

  virtual analysis::InstantiatedType* getInstantiationOfType()
    { return instantiation; }

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL_STMT(ClassDeclNode)

  virtual void print(std::ostream& o, size_t indent = 0);

protected:
  virtual void checkAndInitTypeParams(analysis::SemanticContext* ctx);
  virtual void checkAndInitParents(analysis::SemanticContext* ctx);

  virtual void createClassSymbol(
      const std::string& name,
      analysis::SymbolTable* classTable,
      analysis::Type* type,
      const std::vector<analysis::InstantiatedType*>& typeParams);

private:
  std::vector<analysis::InstantiatedType*> parentTypes;
  std::vector<analysis::InstantiatedType*> typeParamTypes;

  analysis::InstantiatedType* instantiation;
};

}
}

#endif /* VENOM_AST_SYNTHETIC_CLASSDECL_H */
