#ifndef VENOM_AST_ARRAYACCESS_H
#define VENOM_AST_ARRAYACCESS_H

#include <string>

#include <ast/expression/node.h>
#include <util/macros.h>

namespace venom {
namespace ast {

class ArrayAccessNode : public ASTExpressionNode {
public:
  /** Takes ownership of primary and index */
  ArrayAccessNode(ASTExpressionNode* primary, ASTExpressionNode* index)
    : primary(primary), index(index) {}

  ~ArrayAccessNode() {
    delete primary;
    delete index;
  }

  virtual size_t getNumKids() const { return 2; }

  virtual ASTNode* getNthKid(size_t kid) {
    ASTNode *kids[] = {primary, index};
    VENOM_SAFE_RETURN(kids, kid);
  }

  virtual void setNthKid(size_t idx, ASTNode* kid) {
    VENOM_CHECK_RANGE(idx, 2);
    VENOM_SAFE_SET2(primary, index, kid, idx);
  }

  virtual bool needsNewScope(size_t k) const {
    VENOM_CHECK_RANGE(k, 2);
    return false;
  }

  virtual void codeGen(backend::CodeGenerator& cg);

  void codeGenAssignLHS(backend::CodeGenerator& cg, ASTExpressionNode* value);

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL(ArrayAccessNode)

protected:
  virtual analysis::InstantiatedType*
    typeCheckImpl(analysis::SemanticContext* ctx,
                  analysis::InstantiatedType* expected,
                  const analysis::InstantiatedTypeVec& typeParamArgs);

public:
  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(arrayaccess ";
    primary->print(o, indent);
    o << " ";
    index->print(o, indent);
    o << ")";
  }

private:
  ASTExpressionNode* primary;
  ASTExpressionNode* index;
};

}
}

#endif /* VENOM_AST_ARRAYACCESS_H */
