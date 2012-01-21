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
namespace ast {

class FuncDeclNode : public ASTStatementNode {
public:
  /** Takes ownership of params, retTypeString, and stmts */
  FuncDeclNode(const std::string& name,
               const ExprNodeVec& params,
               ASTStatementNode*  stmts)
    : name(name), params(params), stmts(stmts) {
    stmts->addLocationContext(TopLevelFuncBody);
    for (ExprNodeVec::iterator it = this->params.begin();
         it != this->params.end(); ++it) {
      (*it)->addLocationContext(FunctionParam);
    }
  }

  ~FuncDeclNode() {
    util::delete_pointers(params.begin(), params.end());
    delete stmts;
  }

  inline std::string& getName() { return name; }
  inline const std::string& getName() const { return name; }

  inline ASTStatementNode* getStmts() { return stmts; }
  inline const ASTStatementNode* getStmts() const { return stmts; }

  // must call checkAndInitTypeParams() at least once before calling
  virtual std::vector<analysis::InstantiatedType*> getTypeParams() const = 0;

  // must call checkAndInitReturnType() at least once before calling
  virtual analysis::InstantiatedType* getReturnType() const = 0;

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

  virtual void initSymbolTable(analysis::SymbolTable* symbols);

  virtual void registerSymbol(analysis::SemanticContext* ctx);

  virtual analysis::BaseSymbol* getSymbol();

  virtual void typeCheck(analysis::SemanticContext* ctx,
                         analysis::InstantiatedType* expected = NULL);

  virtual void collectInstantiatedTypes(
      std::vector<analysis::InstantiatedType*>& types);

  virtual ASTNode* rewriteLocal(analysis::SemanticContext* ctx,
                                RewriteMode mode);

  virtual void codeGen(backend::CodeGenerator& cg);

  VENOM_AST_TYPED_CLONE_STMT(FuncDeclNode)

protected:
  virtual void checkAndInitTypeParams(analysis::SemanticContext* ctx) = 0;

  virtual void checkAndInitReturnType(analysis::SemanticContext* ctx) = 0;

  std::string       name;
  ExprNodeVec       params;
  ASTStatementNode* stmts;
};

class FuncDeclNodeParser : public FuncDeclNode {
public:
  /** Takes ownership of params, retTypeString, and stmts */
  FuncDeclNodeParser(const std::string&       name,
                     const util::StrVec&      typeParams,
                     const ExprNodeVec&       params,
                     ParameterizedTypeString* retTypeString,
                     ASTStatementNode*        stmts)
    : FuncDeclNode(name, params, stmts),
      typeParams(typeParams),
      retTypeString(retTypeString),
      retType(NULL) {}

  ~FuncDeclNodeParser() {
    if (retTypeString) delete retTypeString;
  }

  virtual std::vector<analysis::InstantiatedType*> getTypeParams() const
    { assert(typeParams.size() == typeParamTypes.size());
      return typeParamTypes; }

  virtual analysis::InstantiatedType* getReturnType() const
    { assert(!retTypeString || retType);
      return retType; }

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL_STMT(FuncDeclNode)

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(func " << name << " -> ";
    if (retTypeString) o << *retTypeString;
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
  virtual void checkAndInitTypeParams(analysis::SemanticContext* ctx);
  virtual void checkAndInitReturnType(analysis::SemanticContext* ctx);

  util::StrVec             typeParams;
  ParameterizedTypeString* retTypeString;

  std::vector<analysis::InstantiatedType*> typeParamTypes;
  analysis::InstantiatedType* retType;
};

class CtorDeclNode : public FuncDeclNodeParser {
public:
  CtorDeclNode(const ExprNodeVec& params,
               ASTStatementNode* stmts,
               const ExprNodeVec& superArgs)
    : FuncDeclNodeParser("<ctor>", util::StrVec(), params, NULL, stmts),
      superArgs(superArgs) {}

  virtual bool isCtor() const { return true; }
  virtual void registerSymbol(analysis::SemanticContext* ctx);

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL_STMT(CtorDeclNode)

private:
  ExprNodeVec superArgs;
};

}
}

#endif /* VENOM_AST_FUNCDECL_H */
