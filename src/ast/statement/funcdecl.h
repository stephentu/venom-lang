/**
 * Copyright (c) 2012 Stephen Tu
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names
 * of its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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

  inline ExprNodeVec& getParams() { return params; }
  inline const ExprNodeVec& getParams() const { return params; }

  inline ASTStatementNode* getStmts() { return stmts; }
  inline const ASTStatementNode* getStmts() const { return stmts; }

  virtual std::vector<std::string> getTypeParamNames() const = 0;

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

  virtual void collectSpecialized(
      analysis::SemanticContext* ctx,
      const analysis::TypeTranslator& t,
      CollectCallback& callback);

  virtual bool isTypeParameterized() const
    { return !getTypeParams().empty(); }

  virtual ASTNode* rewriteLocal(analysis::SemanticContext* ctx,
                                RewriteMode mode);

  virtual void codeGen(backend::CodeGenerator& cg);

  VENOM_AST_TYPED_CLONE_STMT(FuncDeclNode)

protected:
  virtual void checkAndInitTypeParams(analysis::SemanticContext* ctx) = 0;

  virtual void checkAndInitReturnType(analysis::SemanticContext* ctx) = 0;

  ASTStatementNode* cloneForLiftImplHelper(LiftContext& ctx);

  virtual FuncDeclNode* newFuncDeclNodeForLift(
      LiftContext& ctx,
      const std::string& name,
      const ExprNodeVec& params,
      analysis::InstantiatedType* returnType,
      ASTStatementNode* stmts);

  FuncDeclNode* cloneForTemplateImplHelper(const analysis::TypeTranslator& t);

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

  virtual std::vector<std::string> getTypeParamNames() const
    { return typeParams; }

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

protected:
  virtual FuncDeclNode* newFuncDeclNodeForLift(
      LiftContext& ctx,
      const std::string& name,
      const ExprNodeVec& params,
      analysis::InstantiatedType* returnType,
      ASTStatementNode* stmts);

private:
  ExprNodeVec superArgs;
};

}
}

#endif /* VENOM_AST_FUNCDECL_H */
