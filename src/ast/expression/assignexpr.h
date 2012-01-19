#ifndef VENOM_AST_ASSIGNEXPR_H
#define VENOM_AST_ASSIGNEXPR_H

#include <ast/expression/node.h>

namespace venom {
namespace ast {

class AssignExprNode : public ASTExpressionNode {
public:
  /** Takes ownership of variable, value */
  AssignExprNode(ASTExpressionNode* variable, ASTExpressionNode* value)
    : variable(variable), value(value) {
    variable->addLocationContext(AssignmentLHS);
  }

  ~AssignExprNode() {
    delete variable;
    delete value;
  }

  virtual size_t getNumKids() const { return 2; }

  virtual ASTNode* getNthKid(size_t kid) {
    ASTNode *kids[] = {variable, value};
    VENOM_SAFE_RETURN(kids, kid);
  }

  virtual void setNthKid(size_t idx, ASTNode* kid) {
    VENOM_CHECK_RANGE(idx, 2);
    VENOM_SAFE_SET2(variable, value, kid, idx);
  }

  virtual bool needsNewScope(size_t k) const {
    VENOM_CHECK_RANGE(k, 2);
    return false;
  }

  virtual analysis::BaseSymbol* getSymbol();

  virtual void registerSymbol(analysis::SemanticContext* ctx);

  virtual void semanticCheckImpl(analysis::SemanticContext* ctx,
                                 bool doRegister);

  virtual void codeGen(backend::CodeGenerator& cg);

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL(AssignExprNode)

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(assign-expr ";
    variable->print(o, indent);
    o << " ";
    value->print(o, indent);
    o << ")";
  }

protected:
  virtual analysis::InstantiatedType*
    typeCheckImpl(analysis::SemanticContext* ctx,
                  analysis::InstantiatedType* expected,
                  const analysis::InstantiatedTypeVec& typeParamArgs);

private:
  ASTExpressionNode* variable;
  ASTExpressionNode* value;
};

}
}

#endif /* VENOM_AST_ASSIGNEXPR_H */
