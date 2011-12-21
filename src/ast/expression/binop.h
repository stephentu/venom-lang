#ifndef VENOM_AST_BINOP_H
#define VENOM_AST_BINOP_H

#include <string>

#include <ast/expression/node.h>

namespace venom {
namespace ast {

class BinopNode : public ASTExpressionNode {
public:

  enum Type {
    /* numeric ops */
    ADD,
    SUB,
    MULT,
    DIV,
    MOD,

    /* comparison */
    AND,
    OR,
    LT,
    LE,
    GT,
    GE,
    EQ,
    NEQ,

    /* bit ops */
    BIT_AND,
    BIT_OR,
    BIT_XOR,
    BIT_LSHIFT,
    BIT_RSHIFT,
  };

  BinopNode(ASTExpressionNode* left, ASTExpressionNode* right, Type type)
    : left(left), right(right), type(type) {}

  ~BinopNode() {
    delete left;
    delete right;
  }

private:
  ASTExpressionNode* left;
  ASTExpressionNode* right;
  Type type;
};

}
}

#endif /* VENOM_AST_BINOP_H */
