#ifndef VENOM_AST_FUNCDECL_H
#define VENOM_AST_FUNCDECL_H

#include <string>

#include <ast/expression/node.h>
#include <ast/statement/node.h>

#include <util/stl.h>

namespace venom {
namespace ast {

class FuncDeclNode : public ASTStatementNode {
public:
  /** Takes ownership of params and stmts */
  FuncDeclNode(const std::string& name,
               const ExprNodeVec& params,
               const std::string& ret_typename,
               ASTStatementNode*  stmts)
    : name(name), params(params), ret_typename(ret_typename),
      stmts(stmts) {}

  ~FuncDeclNode() {
    util::delete_pointers(params.begin(), params.end());
    delete stmts;
  }

private:
  std::string       name;
  ExprNodeVec       params;
  std::string       ret_typename;
  ASTStatementNode* stmts;
};

}
}

#endif /* VENOM_AST_FUNCDECL_H */
