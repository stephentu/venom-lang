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
    if (ret_typename) delete ret_typename;
    delete stmts;
  }

  inline std::string& getName() { return name; }
  inline const std::string& getName() const { return name; }

  inline ASTStatementNode* getStmts() { return stmts; }
  inline const ASTStatementNode* getStmts() const { return stmts; }

  virtual bool isCtor() const { return false; }

  virtual size_t getNumKids() const { return 1; }

  virtual ASTNode* getNthKid(size_t kid) {
    ASTNode *kids[] = {stmts};
    VENOM_SAFE_RETURN(kids, kid);
  }

  virtual void setNthKid(size_t idx, ASTNode* kid) {
    VENOM_CHECK_RANGE(idx, 1);
    VENOM_SAFE_SET_EXPR(stmts, kid);
  }

  virtual bool needsNewScope(size_t k) const {
    VENOM_CHECK_RANGE(k, 1);
    return true;
  }

  virtual void registerSymbol(analysis::SemanticContext* ctx);

  virtual void typeCheck(analysis::SemanticContext* ctx,
                         analysis::InstantiatedType* expected = NULL);

  //virtual void lift(analysis::SemanticContext* ctx,
  //                  std::vector<ASTStatementNode*>& liftedStmts,
  //                  bool liftThisContext);

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL(FuncDeclNode)

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(func " << name << " -> ";
    if (ret_typename) o << *ret_typename;
    else o << "void";
    o << std::endl << util::indent(indent + 1);
    o << "(params ";
    PrintExprNodeVec(o, params, indent + 1);
    o << ")";
    o << std::endl << util::indent(indent + 1);
    stmts->print(o, indent + 1);
    o << ")";
  }

protected:
  std::string              name;
  util::StrVec             typeParams;
  ExprNodeVec              params;
  ParameterizedTypeString* ret_typename;
  ASTStatementNode*        stmts;

  std::vector<analysis::Type*> typeParamTypes;
};

class CtorDeclNode : public FuncDeclNode {
public:
  CtorDeclNode(const ExprNodeVec& params,
               ASTStatementNode* stmts,
               const ExprNodeVec& superArgs)
    : FuncDeclNode("<ctor>", util::StrVec(), params, NULL, stmts),
      superArgs(superArgs) {}

  virtual bool isCtor() const { return true; }
  virtual void registerSymbol(analysis::SemanticContext* ctx);
private:
  ExprNodeVec superArgs;
};

}
}

#endif /* VENOM_AST_FUNCDECL_H */
