#ifndef VENOM_AST_FUNCTIONCALL_H
#define VENOM_AST_FUNCTIONCALL_H

#include <string>

#include <ast/expression/node.h>
#include <util/stl.h>

namespace venom {
namespace ast {

class FunctionCallNode : public ASTExpressionNode {
public:

  /** Takes ownership of the nodes in primary and args */
  FunctionCallNode(ASTExpressionNode* primary, const ExprNodeVec& args)
    : primary(primary), args(args) {
    primary->setLocationContext(ASTNode::FunctionCall);
  }

  ~FunctionCallNode() {
    delete primary;
    util::delete_pointers(args.begin(), args.end());
  }

  virtual size_t getNumKids() const { return 1 + args.size(); }

  virtual ASTNode* getNthKid(size_t kid) {
    switch (kid) {
    case 0:  return primary;
    default: return args.at(kid - 1);
    }
  }

  virtual bool needsNewScope(size_t k) const { return false; }

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(funccall ";
    primary->print(o, indent);
    o << " ";
    PrintExprNodeVec(o, args, indent);
    o << ")";
  }

private:
  ASTExpressionNode* primary;
  ExprNodeVec        args;
};

}
}

#endif /* VENOM_AST_FUNCTIONCALL_H */
