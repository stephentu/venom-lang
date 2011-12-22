#ifndef VENOM_AST_UNOP_H
#define VENOM_AST_UNOP_H

#include <string>

#include <ast/expression/node.h>

namespace venom {
namespace ast {

class UnopNode : public ASTExpressionNode {
public:

  enum Type {
    /* numeric ops */
    PLUS,
    MINUS,

    /* comparsion */
    CMP_NOT,

    /* bit ops */
    BIT_NOT,
  };

  UnopNode(ASTExpressionNode* kid, Type type)
    : kid(kid), type(type) {}

  ~UnopNode() {
    delete kid;
  }

  virtual void print(std::ostream& o, size_t indent = 0) {
    // TODO: stringify type meaningfully
    o << "(unop " << int(type) << " ";
    kid->print(o, indent);
    o << ")";
  }

private:
  ASTExpressionNode* kid;
  Type type;
};

}
}

#endif /* VENOM_AST_UNOP_H */
