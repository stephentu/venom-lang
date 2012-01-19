#ifndef VENOM_AST_EXPRLIST_H
#define VENOM_AST_EXPRLIST_H

#include <ast/expression/node.h>

#include <util/stl.h>
#include <util/macros.h>

namespace venom {
namespace ast {

class ExprListNode : public ASTExpressionNode {
public:
  ExprListNode() {}
  ExprListNode(const ExprNodeVec& exprs) : exprs(exprs) {}

  ~ExprListNode() {
    util::delete_pointers(exprs.begin(), exprs.end());
  }

  virtual size_t getNumKids() const { return exprs.size(); }

  virtual ASTNode* getNthKid(size_t kid) { return exprs.at(kid); }

  virtual void setNthKid(size_t idx, ASTNode* kid) {
    VENOM_CHECK_RANGE(idx, exprs.size());
    VENOM_SAFE_SET_EXPR(exprs[idx], kid);
  }

  virtual bool needsNewScope(size_t k) const {
    VENOM_CHECK_RANGE(k, exprs.size());
    return false;
  }

  virtual analysis::BaseSymbol* getSymbol() {
    return exprs[exprs.size() - 1]->getSymbol();
  }

  virtual void codeGen(backend::CodeGenerator& cg);

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL(ExprListNode)

protected:
  virtual analysis::InstantiatedType*
    typeCheckImpl(analysis::SemanticContext* ctx,
                  analysis::InstantiatedType* expected,
                  const analysis::InstantiatedTypeVec& typeParamArgs);

public:
  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(exprs" << std::endl << util::indent(indent + 1);
    PrintExprNodeVec(o, exprs, indent + 1);
    o << ")";
  }

private:
  ExprNodeVec exprs;
};

}
}

#endif /* VENOM_AST_EXPRLIST_H */
