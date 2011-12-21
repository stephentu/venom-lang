#ifndef VENOM_AST_STRINGLITERAL_H
#define VENOM_AST_STRINGLITERAL_H

#include <string>

#include <ast/expression/node.h>

namespace venom {
namespace ast {

class StringLiteralNode : public ASTExpressionNode {
public:
  StringLiteralNode(const std::string& value) : value(value) {}
private:
  std::string value;
};

}
}

#endif /* VENOM_AST_STRINGLITERAL_H */
