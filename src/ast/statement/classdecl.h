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

#ifndef VENOM_AST_CLASSDECL_H
#define VENOM_AST_CLASSDECL_H

#include <iostream>
#include <string>
#include <vector>

#include <ast/expression/node.h>
#include <ast/statement/node.h>

#include <util/stl.h>
#include <util/macros.h>

namespace venom {

/** Forward decl **/
namespace analysis {
  class ClassSymbol;
  class Type;
}

namespace ast {

/** Forward decl */
class StmtListNode;

class ClassDeclNode : public ASTStatementNode {
public:
  ClassDeclNode(const std::string& name,
                ASTStatementNode* stmts,
                analysis::InstantiatedType* instantiation)
    : name(name), stmts(stmts), instantiation(instantiation) {
    stmts->addLocationContext(ASTNode::TopLevelClassBody);
  }

  ~ClassDeclNode() {
    delete stmts;
  }

  inline std::string& getName() { return name; }
  inline const std::string& getName() const { return name; }

  StmtListNode* getStmts();
  inline const StmtListNode* getStmts() const
    { return const_cast<ClassDeclNode*>(this)->getStmts(); }

  analysis::ClassSymbol* getClassSymbol();
  inline const analysis::ClassSymbol* getClassSymbol() const
    { return const_cast<ClassDeclNode*>(this)->getClassSymbol(); }

  // must call checkAndInitParents() at least once before calling
  virtual std::vector<analysis::InstantiatedType*> getParents() const = 0;

  virtual std::vector<std::string> getTypeParamNames() const = 0;

  // must call checkAndInitTypeParams() at least once before calling
  virtual std::vector<analysis::InstantiatedType*> getTypeParams() const = 0;

  // the parameterized type which this ClassDeclNode instantiates
  // NULL if none
  inline analysis::InstantiatedType* getInstantiationOfType()
    { return instantiation; }
  inline const analysis::InstantiatedType* getInstantiationOfType() const
    { return instantiation; }

  /** Returns the NON-specialized self-type */
  analysis::InstantiatedType* getSelfType(analysis::SemanticContext* ctx);

  /** Is this class itself a nested class */
  inline bool isNestedClass() const
    { return hasLocationContext(TopLevelClassBody); }

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

  virtual void semanticCheckImpl(analysis::SemanticContext* ctx,
                                 bool doRegister);

  virtual void collectSpecialized(
      analysis::SemanticContext* ctx,
      const analysis::TypeTranslator& t,
      CollectCallback& callback);

  virtual bool isTypeParameterized() const
    { return !getTypeParams().empty(); }

  virtual void codeGen(backend::CodeGenerator& cg);

  virtual analysis::BaseSymbol* getSymbol();

  VENOM_AST_TYPED_CLONE_STMT(ClassDeclNode)

protected:
  virtual void checkAndInitTypeParams(analysis::SemanticContext* ctx) = 0;

  virtual void checkAndInitParents(analysis::SemanticContext* ctx) = 0;

  void createClassSymbol(
      const std::string& name,
      analysis::SymbolTable* classTable,
      analysis::Type* type,
      const std::vector<analysis::InstantiatedType*>& typeParams);

  void registerClassSymbol(
      analysis::SemanticContext* ctx,
      const std::vector<analysis::InstantiatedType*>& parentTypes,
      const std::vector<analysis::InstantiatedType*>& typeParamTypes);

  virtual void liftPhaseImpl(analysis::SemanticContext* ctx,
                             analysis::SymbolTable* liftInto,
                             std::vector<ASTStatementNode*>& liftedStmts,
                             bool excludeFunctions);

  ASTStatementNode* cloneForLiftImplHelper(LiftContext& ctx);

  ClassDeclNode* cloneForTemplateImplHelper(const analysis::TypeTranslator& t);

  std::string       name;
  ASTStatementNode* stmts;

  analysis::InstantiatedType* instantiation;
};

/** comes from the parser */
class ClassDeclNodeParser : public ClassDeclNode {
public:
  /** Takes ownership of parents */
  ClassDeclNodeParser(const std::string&   name,
                      const TypeStringVec& parents,
                      const util::StrVec&  typeParams,
                      ASTStatementNode*    stmts,
                      analysis::InstantiatedType* instantiation = NULL)
    : ClassDeclNode(name, stmts, instantiation),
      parents(parents), typeParams(typeParams) {}

  ~ClassDeclNodeParser() {
    util::delete_pointers(parents.begin(), parents.end());
  }

  virtual std::vector<analysis::InstantiatedType*> getParents() const
    { assert(parents.empty() ?
               parentTypes.size() == 1 :
               parents.size() == parentTypes.size());
      return parentTypes; }

  virtual std::vector<std::string> getTypeParamNames() const
    { return typeParams; }

  virtual std::vector<analysis::InstantiatedType*> getTypeParams() const
    { assert(typeParams.size() == typeParamTypes.size());
      return typeParamTypes; }

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL_STMT(ClassDeclNode)

  virtual void print(std::ostream& o, size_t indent = 0);

protected:
  virtual void checkAndInitTypeParams(analysis::SemanticContext* ctx);
  virtual void checkAndInitParents(analysis::SemanticContext* ctx);

private:
  TypeStringVec parents;
  util::StrVec  typeParams;

  std::vector<analysis::InstantiatedType*> parentTypes;
  std::vector<analysis::InstantiatedType*> typeParamTypes;
};

}
}

#endif /* VENOM_AST_CLASSDECL_H */
