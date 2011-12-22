#ifndef VENOM_AST_ASSIGN_H
#define VENOM_AST_ASSIGN_H

#include <string>

#include <ast/expression/node.h>
#include <ast/statement/node.h>

#include <util/macros.h>

namespace venom {
namespace ast {

class AssignNode : public ASTStatementNode {
public:
  /** Takes ownership of variable, value */
  AssignNode(ASTExpressionNode* variable, ASTExpressionNode* value)
    : variable(variable), value(value) {}

  ~AssignNode() {
    delete variable;
    delete value;
  }

  virtual size_t getNumKids() const { return 2; }

  virtual ASTNode* getNthKid(size_t kid) {
    ASTNode *kids[] = {variable, value};
    VENOM_SAFE_RETURN(kids, kid);
  }

  virtual bool needsNewScope(size_t k) const { return false; }

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(assign ";
    variable->print(o, indent);
    o << " ";
    value->print(o, indent);
    o << ")";
  }

private:
  ASTExpressionNode* variable;
  ASTExpressionNode* value;
};

}
}

#endif /* VENOM_AST_ASSIGN_H */
