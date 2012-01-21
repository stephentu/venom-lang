#ifndef VENOM_AST_ARRAYLITERAL_H
#define VENOM_AST_ARRAYLITERAL_H

#include <ast/expression/node.h>
#include <util/stl.h>

namespace venom {
namespace ast {

class ArrayLiteralNode : public ASTExpressionNode {
public:
  /** Takes ownership of the nodes in values */
  ArrayLiteralNode(const ExprNodeVec& values)
    : values(values) {}

  ~ArrayLiteralNode() {
    util::delete_pointers(values.begin(), values.end());
  }

  virtual size_t getNumKids() const { return values.size(); }

  virtual ASTNode* getNthKid(size_t kid) {
    VENOM_CHECK_RANGE(kid, values.size());
    return values[kid];
  }

  virtual void setNthKid(size_t idx, ASTNode* kid) {
    VENOM_CHECK_RANGE(idx, values.size());
    VENOM_SAFE_SET_EXPR(values[idx], kid);
  }

  virtual bool needsNewScope(size_t k) const {
    VENOM_CHECK_RANGE(k, values.size());
    return false;
  }

  virtual ASTNode* rewriteLocal(analysis::SemanticContext* ctx,
                                RewriteMode mode);

  virtual void codeGen(backend::CodeGenerator& cg) {
    // is re-written, so never need to code-generate it
    VENOM_NOT_REACHED;
  }

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL_EXPR(ArrayLiteralNode)

protected:
  virtual analysis::InstantiatedType*
    typeCheckImpl(analysis::SemanticContext* ctx,
                  analysis::InstantiatedType* expected,
                  const analysis::InstantiatedTypeVec& typeParamArgs);

public:
  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(arrayliteral ";
    PrintExprNodeVec(o, values, indent);
    o << ")";
  }

private:
  ExprNodeVec values;
};

}
}

#endif /* VENOM_AST_ARRAYLITERAL_H */
