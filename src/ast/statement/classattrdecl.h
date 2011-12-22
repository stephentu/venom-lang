#ifndef VENOM_AST_CLASSATTRDECL_H
#define VENOM_AST_CLASSATTRDECL_H

#include <ast/expression/node.h>
#include <ast/statement/node.h>

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
private:
  ASTExpressionNode* variable;
  ASTExpressionNode* value;
};

}
}

#endif /* VENOM_AST_CLASSATTRDECL_H */
