#ifndef VENOM_AST_VARIABLE_H
#define VENOM_AST_VARIABLE_H

#include <string>

#include <ast/expression/node.h>

namespace venom {
namespace ast {

class VariableNode : public ASTExpressionNode {
public:
  VariableNode(const std::string& name,
               const std::string& explicit_type_name = "")
    : name(name), explicit_type_name(explicit_type_name) {}

private:
  std::string name;
  std::string explicit_type_name;
};

class VariableSelfNode : public VariableNode {
public:
  VariableSelfNode()
    : VariableNode("self") {}
};

}
}

#endif /* VENOM_AST_VARIABLE_H */
