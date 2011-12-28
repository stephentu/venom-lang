#ifndef VENOM_AST_ATTRACCESS_H
#define VENOM_AST_ATTRACCESS_H

#include <string>

#include <ast/expression/node.h>
#include <util/macros.h>

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

  virtual size_t getNumKids() const { return 1; }

  virtual ASTNode* getNthKid(size_t kid) {
    ASTNode *kids[] = {primary};
    VENOM_SAFE_RETURN(kids, kid);
  }

  virtual bool needsNewScope(size_t k) const { return false; }

  virtual analysis::InstantiatedType*
    typeCheck(analysis::SemanticContext* ctx,
              analysis::InstantiatedType* expected = NULL,
              const analysis::InstantiatedTypeVec& typeParamArgs
                = analysis::InstantiatedTypeVec());

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
