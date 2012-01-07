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
  FunctionCallNode(ASTExpressionNode*   primary,
                   const TypeStringVec& typeArgs,
                   const ExprNodeVec&   args)
    : primary(primary), typeArgs(typeArgs), args(args) {
    primary->setLocationContext(ASTNode::FunctionCall);
  }

  ~FunctionCallNode() {
    delete primary;
    util::delete_pointers(args.begin(), args.end());
  }

  virtual size_t getNumKids() const { return 1 + args.size(); }

  virtual ASTNode* getNthKid(size_t kid) {
    VENOM_CHECK_RANGE(kid, 1 + args.size());
    switch (kid) {
    case 0:  return primary;
    default: return args[kid - 1];
    }
  }

  virtual void setNthKid(size_t idx, ASTNode* kid) {
    VENOM_CHECK_RANGE(idx, 1 + args.size());
    switch (idx) {
    case 0:  VENOM_SAFE_SET_CASE(primary, kid);
    default: VENOM_SAFE_SET_CASE(args[idx - 1], kid);
    }
  }

  virtual bool needsNewScope(size_t k) const {
    VENOM_CHECK_RANGE(k, 1 + args.size());
    return false;
  }

protected:
  virtual analysis::InstantiatedType*
    typeCheckImpl(analysis::SemanticContext* ctx,
                  analysis::InstantiatedType* expected,
                  const analysis::InstantiatedTypeVec& typeParamArgs);

public:
  virtual void codeGen(backend::CodeGenerator& cg);

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(funccall ";
    primary->print(o, indent);
    o << " ";
    PrintExprNodeVec(o, args, indent);
    o << ")";
  }

private:
  ASTExpressionNode* primary;
  TypeStringVec      typeArgs;
  ExprNodeVec        args;
};

}
}

#endif /* VENOM_AST_FUNCTIONCALL_H */
