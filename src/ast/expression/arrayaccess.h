#ifndef VENOM_AST_ARRAYACCESS_H
#define VENOM_AST_ARRAYACCESS_H

#include <string>

#include <ast/expression/node.h>
#include <util/macros.h>

namespace venom {
namespace ast {

class ArrayAccessNode : public ASTExpressionNode {
public:
  /** Takes ownership of primary and index */
  ArrayAccessNode(ASTExpressionNode* primary, ASTExpressionNode* index)
    : primary(primary), index(index) {}

  ~ArrayAccessNode() {
    delete primary;
    delete index;
  }

  virtual size_t getNumKids() const { return 2; }

  virtual ASTNode* getNthKid(size_t kid) {
    ASTNode *kids[] = {primary, index};
    VENOM_SAFE_RETURN(kids, kid);
  }

  virtual bool needsNewScope(size_t k) const { return false; }

  virtual analysis::InstantiatedType*
    typeCheck(analysis::SemanticContext* ctx);

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(arrayaccess ";
    primary->print(o, indent);
    o << " ";
    index->print(o, indent);
    o << ")";
  }

private:
  ASTExpressionNode* primary;
  ASTExpressionNode* index;
};

}
}

#endif /* VENOM_AST_ARRAYACCESS_H */
