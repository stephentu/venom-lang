#ifndef VENOM_AST_CLASSATTRDECL_H
#define VENOM_AST_CLASSATTRDECL_H

#include <ast/expression/node.h>
#include <ast/statement/node.h>

#include <util/macros.h>

namespace venom {
namespace ast {

class ClassAttrDeclNode : public ASTStatementNode {
public:
  /** Takes ownership of variable and value */
  ClassAttrDeclNode(ASTExpressionNode* variable,
                    ASTExpressionNode* value)
    : variable(variable), value(value) {}

  ~ClassAttrDeclNode() {
    delete variable;
    if (value) delete value;
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

  virtual void registerSymbol(analysis::SemanticContext* ctx);

  virtual void semanticCheckImpl(analysis::SemanticContext* ctx,
                                 bool doRegister);

  virtual void typeCheck(analysis::SemanticContext* ctx,
                         analysis::InstantiatedType* expected);

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL(ClassAttrDeclNode)

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(attr ";
    variable->print(o, indent);
    if (value) {
      o << " ";
      value->print(o, indent);
    }
    o << ")";
  }

private:
  ASTExpressionNode* variable;
  ASTExpressionNode* value;
};

}
}

#endif /* VENOM_AST_CLASSATTRDECL_H */
