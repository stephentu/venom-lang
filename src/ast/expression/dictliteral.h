#ifndef VENOM_AST_DICTLITERAL_H
#define VENOM_AST_DICTLITERAL_H

#include <vector>
#include <utility>

#include <ast/expression/node.h>
#include <util/macros.h>
#include <util/stl.h>

namespace venom {
namespace ast {

class DictPair :
  public ASTExpressionNode,
  public std::pair< ASTExpressionNode*, ASTExpressionNode* > {
public:
  DictPair(ASTExpressionNode* key, ASTExpressionNode* value)
    : std::pair< ASTExpressionNode*, ASTExpressionNode* >(key, value) {}

  ~DictPair() {
    delete first;
    delete second;
  }

  inline ASTExpressionNode* key()   { return first;  }
  inline ASTExpressionNode* value() { return second; }

  virtual size_t getNumKids() const { return 2; }

  virtual ASTNode* getNthKid(size_t kid) {
    ASTNode *kids[] = {first, second};
    VENOM_SAFE_RETURN(kids, kid);
  }

  virtual void setNthKid(size_t idx, ASTNode* kid) {
    VENOM_CHECK_RANGE(idx, 2);
    VENOM_SAFE_SET2(first, second, kid, idx);
  }

  virtual bool needsNewScope(size_t k) const {
    VENOM_CHECK_RANGE(k, 2);
    return false;
  }

  virtual ASTNode* rewriteLocal(analysis::SemanticContext* ctx,
                                RewriteMode mode) {
    return ASTNode::rewriteLocal(ctx, mode);
  }

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL(DictPair)

protected:
  virtual analysis::InstantiatedType*
    typeCheckImpl(analysis::SemanticContext* ctx,
                  analysis::InstantiatedType* expected,
                  const analysis::InstantiatedTypeVec& typeParamArgs) {
    VENOM_UNIMPLEMENTED;
  }

public:
  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(pair ";
    key()->print(o, indent);
    o << " ";
    value()->print(o, indent);
    o << ")";
  }

};

typedef std::vector<DictPair*> DictPairVec;

inline DictPairVec* MakeDictPairVec1(DictPair* pair) {
  DictPairVec *v = new DictPairVec;
  v->push_back(pair);
  return v;
}

class DictLiteralNode : public ASTExpressionNode {
public:

  /** Takes ownership of the nodes in the pairs */
  DictLiteralNode(const DictPairVec& pairs)
    : pairs(pairs) {}

  ~DictLiteralNode() {
    util::delete_pointers(pairs.begin(), pairs.end());
  }

  virtual size_t getNumKids() const { return pairs.size(); }

  virtual ASTNode* getNthKid(size_t kid) {
    VENOM_CHECK_RANGE(kid, pairs.size());
    return pairs[kid];
  }

  virtual void setNthKid(size_t idx, ASTNode* kid) {
    VENOM_CHECK_RANGE(idx, pairs.size());
    VENOM_SAFE_SET_EXPR(pairs[idx], kid);
  }

  virtual bool needsNewScope(size_t k) const {
    VENOM_CHECK_RANGE(k, pairs.size());
    return false;
  }

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL(DictLiteralNode)

protected:
  virtual analysis::InstantiatedType*
    typeCheckImpl(analysis::SemanticContext* ctx,
                  analysis::InstantiatedType* expected,
                  const analysis::InstantiatedTypeVec& typeParamArgs);

public:
  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(dictliteral ";
    // TODO: abstract this away with Print{Expr,Stmt}NodeVec
    for (DictPairVec::iterator it = pairs.begin();
         it != pairs.end(); ++it) {
      (*it)->print(o, indent);
      if (it + 1 != pairs.end()) o << " ";

    }
    o << ")";
  }

private:
  DictPairVec pairs;
};

}
}

#endif /* VENOM_AST_DICTLITERAL_H */

