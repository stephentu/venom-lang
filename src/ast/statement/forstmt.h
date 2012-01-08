#ifndef VENOM_AST_FORSTMT_H
#define VENOM_AST_FORSTMT_H

#include <iostream>

#include <ast/expression/node.h>
#include <ast/statement/node.h>

#include <util/macros.h>

namespace venom {
namespace ast {

class ForStmtNode : public ASTStatementNode {
public:
  /** Takes ownership of variable, expr, stmts */
  ForStmtNode(ASTExpressionNode* variable,
              ASTExpressionNode* expr,
              ASTStatementNode*  stmts)
    : variable(variable), expr(expr), stmts(stmts) {}

  ~ForStmtNode() {
    delete variable;
    delete expr;
    delete stmts;
  }

  virtual size_t getNumKids() const { return 3; }

  virtual ASTNode* getNthKid(size_t kid) {
    ASTNode *kids[] = {variable, expr, stmts};
    VENOM_SAFE_RETURN(kids, kid);
  }

  virtual void setNthKid(size_t idx, ASTNode* kid) {
    VENOM_CHECK_RANGE(idx, 3);
    VENOM_SAFE_SET3(variable, expr, stmts, kid, idx);
  }

  virtual bool needsNewScope(size_t k) const {
    VENOM_CHECK_RANGE(k, 3);
    return k == 2;
  }

  virtual void semanticCheckImpl(analysis::SemanticContext* ctx,
                                 bool doRegister);

  virtual void registerSymbol(analysis::SemanticContext* ctx);

  virtual void typeCheck(analysis::SemanticContext* ctx,
                         analysis::InstantiatedType* expected);

  virtual ASTNode* rewriteLocal(analysis::SemanticContext* ctx);

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(for ";
    variable->print(o, indent);
    o << " ";
    expr->print(o, indent);
    o << std::endl << util::indent(indent + 1);
    stmts->print(o, indent + 1);
    o << ")";
  }

private:
  ASTExpressionNode* variable;
  ASTExpressionNode* expr;
  ASTStatementNode*  stmts;
};

}
}

#endif /* VENOM_AST_FORSTMT_H */
