#ifndef VENOM_AST_ASSIGN_H
#define VENOM_AST_ASSIGN_H

#include <string>

#include <ast/expression/node.h>
#include <ast/statement/node.h>

#include <util/macros.h>

namespace venom {

namespace analysis {
  /** Forward decl */
  class ClassSymbol;
}

namespace ast {

class AssignNode : public ASTStatementNode {
  friend class ClassAttrDeclNode;
public:
  /** Takes ownership of variable, value */
  AssignNode(ASTExpressionNode* variable, ASTExpressionNode* value)
    : variable(variable), value(value) {
    variable->addLocationContext(AssignmentLHS);
  }

  ~AssignNode() {
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

  virtual void registerSymbol(analysis::SemanticContext* ctx);

  //virtual ASTNode* rewriteLocal(analysis::SemanticContext* ctx,
  //                              RewriteMode mode);

  virtual void semanticCheckImpl(analysis::SemanticContext* ctx,
                                 bool doRegister);

  virtual void typeCheck(analysis::SemanticContext* ctx,
                         analysis::InstantiatedType* expected = NULL);

  virtual void codeGen(backend::CodeGenerator& cg);

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL(AssignNode)

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(assign ";
    variable->print(o, indent);
    o << " ";
    value->print(o, indent);
    o << ")";
  }

protected:
  static void
  TypeCheckAssignment(analysis::SemanticContext* ctx,
                      analysis::SymbolTable*     symbols,
                      ASTExpressionNode*         variable,
                      ASTExpressionNode*         value,
                      analysis::ClassSymbol*     classSymbol = NULL);

private:
  ASTExpressionNode* variable;
  ASTExpressionNode* value;
};

}
}

#endif /* VENOM_AST_ASSIGN_H */
