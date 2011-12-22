#ifndef VENOM_AST_STRINGLITERAL_H
#define VENOM_AST_STRINGLITERAL_H

#include <string>

#include <ast/expression/node.h>

namespace venom {
namespace ast {

class StringLiteralNode : public ASTExpressionNode {
public:
  StringLiteralNode(const std::string& value) : value(value) {}

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(stringlit '" << value << "')";
  }

private:
  std::string value;
};

}
}

#endif /* VENOM_AST_STRINGLITERAL_H */
