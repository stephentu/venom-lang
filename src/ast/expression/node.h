#ifndef VENOM_AST_EXPRESSION_NODE_H
#define VENOM_AST_EXPRESSION_NODE_H

#include <iostream>
#include <string>
#include <vector>

#include <ast/node.h>
#include <util/stl.h>

namespace venom {

namespace analysis {
  /** Forward decl for typeCheck */
  class InstantiatedType;
  typedef std::vector<InstantiatedType*> InstantiatedTypeVec;
}

namespace ast {

/** This contains the structure of an un-resolved type,
 * which we get from the parser. The reason we don't use
 * Type objects right away is because Type objects represent
 * properly resolved types, which we cannot get right away
 * from the parser */
struct ParameterizedTypeString {
  ParameterizedTypeString(const std::string& name)
    : name(name) {}
  ParameterizedTypeString(const std::string& name,
                          const std::vector<ParameterizedTypeString*>& params)
    : name(name), params(params) {}
  ~ParameterizedTypeString() {
    util::delete_pointers(params.begin(), params.end());
  }

  std::string stringify() const;

  const std::string                           name;
  const std::vector<ParameterizedTypeString*> params;
};

typedef std::vector<ParameterizedTypeString*> TypeStringVec;

class ASTExpressionNode : public ASTNode {
public:
  ASTExpressionNode() {}

  /** Do type-checking on this node recursively.
   *  expected is what you expect this node to type-check to, if it matters
   *    (is NULL in most cases)
   *  typeParamArgs is a list of type parameter arguments, if it matters
   *    (is empty in most cases)
   */
  virtual analysis::InstantiatedType*
    typeCheck(analysis::SemanticContext* ctx,
              analysis::InstantiatedType* expected = NULL,
              const analysis::InstantiatedTypeVec& typeParamArgs
                = analysis::InstantiatedTypeVec()) = 0;
};

typedef std::vector<ASTExpressionNode *> ExprNodeVec;

inline ExprNodeVec * MakeExprVec1(ASTExpressionNode *a0) {
  ExprNodeVec *v = new ExprNodeVec;
  v->push_back(a0);
  return v;
}

inline void PrintExprNodeVec(std::ostream& o,
                             const ExprNodeVec& exprs,
                             size_t indent) {
  o << "(";
  for (ExprNodeVec::const_iterator it = exprs.begin();
       it != exprs.end(); ++it) {
    (*it)->print(o, indent);
    if (it + 1 != exprs.end()) o << " ";
  }
  o << ")";
}

}
}

inline std::ostream& operator<<(std::ostream& o,
                                const venom::ast::ParameterizedTypeString& t) {
  o << t.stringify();
  return o;
}

#endif /* VENOM_AST_EXPRESSION_NODE_H */
