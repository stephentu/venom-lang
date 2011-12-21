#ifndef VENOM_AST_DICTLITERAL_H
#define VENOM_AST_DICTLITERAL_H

#include <vector>
#include <utility>

#include <ast/expression/node.h>

namespace venom {
namespace ast {

class DictPair :
  public std::pair< ASTExpressionNode*, ASTExpressionNode* > {
public:
  DictPair(ASTExpressionNode* key, ASTExpressionNode* value)
    : std::pair< ASTExpressionNode*, ASTExpressionNode* >(key, value) {}
  inline ASTExpressionNode* key()   { return first;  }
  inline ASTExpressionNode* value() { return second; }
};

typedef std::vector<DictPair> DictPairVec;

inline DictPairVec* MakeDictPairVec1(const DictPair& pair) {
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
    for (DictPairVec::iterator it = pairs.begin();
         it != pairs.end(); ++it) {
      delete it->key();
      delete it->value();
    }
  }
private:
  DictPairVec pairs;
};

}
}

#endif /* VENOM_AST_DICTLITERAL_H */

