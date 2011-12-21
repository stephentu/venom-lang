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

class VariableSelfNode : public VariableNode {
public:
  VariableSelfNode()
    : VariableNode("self") {}
};

}
}

#endif /* VENOM_AST_VARIABLE_H */
