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

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(attraccess ";
    primary->print(o, indent);
    o << " " << name << ")";
  }

private:
  ASTExpressionNode* primary;
  std::string name;
};

}
}

#endif /* VENOM_AST_ATTRACCESS_H */
