#ifndef VENOM_AST_BINOP_H
#define VENOM_AST_BINOP_H

#include <string>

#include <ast/expression/node.h>
#include <util/macros.h>

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
    CMP_AND,
    CMP_OR,
    CMP_LT,
    CMP_LE,
    CMP_GT,
    CMP_GE,
    CMP_EQ,
    CMP_NEQ,

    /* bit ops */
    BIT_AND,
    BIT_OR,
    BIT_XOR,
    BIT_LSHIFT,
    BIT_RSHIFT,
  };

  static std::string StringifyType(Type type);

  BinopNode(ASTExpressionNode* left, ASTExpressionNode* right, Type type)
    : left(left), right(right), type(type) {}

  ~BinopNode() {
    delete left;
    delete right;
  }

  bool isShortCircuitOp() const;

  virtual size_t getNumKids() const { return 2; }

  virtual ASTNode* getNthKid(size_t kid) {
    ASTNode *kids[] = {left, right};
    VENOM_SAFE_RETURN(kids, kid);
  }

  virtual bool needsNewScope(size_t k) const { return false; }

protected:
  virtual analysis::InstantiatedType*
    typeCheckImpl(analysis::SemanticContext* ctx,
                  analysis::InstantiatedType* expected,
                  const analysis::InstantiatedTypeVec& typeParamArgs);

public:
  virtual void codeGen(backend::CodeGenerator& cg);

  virtual void print(std::ostream& o, size_t indent = 0) {
    // TODO: stringify the type
    o << "(binop " << int(type) << " ";
    left->print(o, indent);
    o << " ";
    right->print(o, indent);
    o << ")";
  }

private:
  ASTExpressionNode* left;
  ASTExpressionNode* right;
  Type type;
};

}
}

#endif /* VENOM_AST_BINOP_H */
