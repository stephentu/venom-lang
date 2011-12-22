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

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(ident " << name;
    if (!explicit_type_name.empty()) o << " " << explicit_type_name;
    o << ")";
  }

private:
  std::string name;
  std::string explicit_type_name;
};

class VariableSelfNode : public VariableNode {
public:
  VariableSelfNode()
    : VariableNode("self") {}

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(self)";
  }
};

}
}

#endif /* VENOM_AST_VARIABLE_H */
