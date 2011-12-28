#ifndef VENOM_AST_FUNCDECL_H
#define VENOM_AST_FUNCDECL_H

#include <iostream>
#include <string>
#include <vector>

#include <ast/expression/node.h>
#include <ast/statement/node.h>

#include <util/stl.h>
#include <util/macros.h>

namespace venom {

namespace analysis {
  /** Forward decl */
  class Type;
}

namespace ast {

class FuncDeclNode : public ASTStatementNode {
public:
  /** Takes ownership of params, ret_typename, and stmts */
  FuncDeclNode(const std::string&       name,
               const util::StrVec&      typeParams,
               const ExprNodeVec&       params,
               ParameterizedTypeString* ret_typename,
               ASTStatementNode*        stmts)
    : name(name), typeParams(typeParams), params(params),
      ret_typename(ret_typename), stmts(stmts) {}

  ~FuncDeclNode() {
    util::delete_pointers(params.begin(), params.end());
    delete ret_typename;
    delete stmts;
  }

  inline std::string& getName() { return name; }
  inline const std::string& getName() const { return name; }

  virtual size_t getNumKids() const { return 1; }

  virtual ASTNode* getNthKid(size_t kid) {
    ASTNode *kids[] = {stmts};
    VENOM_SAFE_RETURN(kids, kid);
  }

  virtual bool needsNewScope(size_t k) const { return true; }

  virtual void registerSymbol(analysis::SemanticContext* ctx);

  virtual void typeCheck(analysis::SemanticContext* ctx,
                         analysis::InstantiatedType* expected = NULL);

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(func " << name << " -> " << *ret_typename;
    o << std::endl << util::indent(indent + 1);
    o << "(params ";
    PrintExprNodeVec(o, params, indent + 1);
    o << ")";
    o << std::endl << util::indent(indent + 1);
    stmts->print(o, indent + 1);
    o << ")";
  }

private:
  std::string              name;
  util::StrVec             typeParams;
  ExprNodeVec              params;
  ParameterizedTypeString* ret_typename;
  ASTStatementNode*        stmts;

  std::vector<analysis::Type*> typeParamTypes;
};

}
}

#endif /* VENOM_AST_FUNCDECL_H */
