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
    : primary(primary), args(args) {}

  ~FunctionCallNode() {
    delete primary;
    util::delete_pointers(args.begin(), args.end());
  }
private:
  ASTExpressionNode* primary;
  ExprNodeVec        args;
};

}
}

#endif /* VENOM_AST_FUNCTIONCALL_H */
