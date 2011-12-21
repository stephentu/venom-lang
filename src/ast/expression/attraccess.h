#ifndef VENOM_AST_ATTRACCESS_H
#define VENOM_AST_ATTRACCESS_H

#include <string>

#include <ast/expression/node.h>

namespace venom {
namespace ast {

class AttrAccessNode : public ASTExpressionNode {
public:
  /** Takes ownership of primary */
  AttrAccessNode(ASTExpressionNode* primary, const std::string& name)
    : primary(primary), name(name) {}

  ~AttrAccessNode() {
    delete primary;
  }
private:
  ASTExpressionNode* primary;
  std::string name;
};

}
}

#endif /* VENOM_AST_ATTRACCESS_H */
