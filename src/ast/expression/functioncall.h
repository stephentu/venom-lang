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
