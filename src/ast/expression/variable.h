#ifndef VENOM_AST_VARIABLE_H
#define VENOM_AST_VARIABLE_H

#include <string>

#include <ast/expression/node.h>

namespace venom {
namespace ast {

class VariableNode : public ASTExpressionNode {
public:
  VariableNode(const std::string& name)
    : name(name) {}

private:
  std::string name;
};

}
}

#endif /* VENOM_AST_VARIABLE_H */

