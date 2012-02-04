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
 * 3. Neither the name of the PostgreSQL Global Development Group nor the names
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

#ifndef VENOM_AST_VARIABLE_H
#define VENOM_AST_VARIABLE_H

#include <cassert>
#include <stdexcept>
#include <string>
#include <vector>

#include <ast/expression/node.h>
#include <analysis/typetranslator.h>

namespace venom {
namespace ast {

class VariableNode : public ASTExpressionNode {
public:
  VariableNode(const std::string& name)
    : name(name), symbol(NULL) { assert(!name.empty()); }

  inline std::string& getName() { return name; }
  inline const std::string& getName() const { return name; }

  /** Returns NULL if no explicit type */
  virtual analysis::InstantiatedType* getExplicitType() = 0;

  virtual size_t getNumKids() const { return 0; }

  virtual ASTNode* getNthKid(size_t kid) {
    throw std::out_of_range(VENOM_SOURCE_INFO);
  }

  virtual void setNthKid(size_t idx, ASTNode* kid) {
    throw std::out_of_range(VENOM_SOURCE_INFO);
  }

  virtual bool needsNewScope(size_t k) const {
    throw std::out_of_range(VENOM_SOURCE_INFO);
  }

  virtual analysis::BaseSymbol* getSymbol() { return symbol; }

  virtual void registerSymbol(analysis::SemanticContext* ctx);

  virtual void collectNonLocalRefs(LiftContext& ctx);

  virtual ASTNode* rewriteAfterLift(
      const LiftContext::LiftMap& liftMap,
      const std::set<analysis::BaseSymbol*>& refs);

  virtual ASTNode* rewriteLocal(analysis::SemanticContext* ctx,
                                RewriteMode mode);

  virtual void codeGen(backend::CodeGenerator& cg);

  VENOM_AST_TYPED_CLONE_EXPR(VariableNode)

protected:
  virtual analysis::InstantiatedType*
    typeCheckImpl(analysis::SemanticContext* ctx,
                  analysis::InstantiatedType* expected,
                  const analysis::InstantiatedTypeVec& typeParamArgs);

  bool isNonLocalRef(analysis::SymbolTable* definedIn,
                     analysis::Symbol*& nonLocalSym);

  std::string name;

  analysis::BaseSymbol* symbol;
  analysis::TypeTranslator translator;
};

class VariableNodeParser : public VariableNode {
public:
  /** Takes ownership of explicitTypeString */
  VariableNodeParser(const std::string&       name,
                     ParameterizedTypeString* explicitTypeString)
    : VariableNode(name),
      explicitTypeString(explicitTypeString),
      explicitType(NULL) {}

  ~VariableNodeParser() {
    if (explicitTypeString) delete explicitTypeString;
  }

  inline const ParameterizedTypeString*
    getExplicitParameterizedTypeString() const { return explicitTypeString; }

  virtual analysis::InstantiatedType* getExplicitType();

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL_EXPR(VariableNode)

public:
  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(ident " << name;
    if (explicitTypeString) o << " " << *explicitTypeString;
    if (symbol) o << " (sym-addr " << std::hex << symbol << ")";
    o << ")";
  }

private:
  ParameterizedTypeString* explicitTypeString;
  analysis::InstantiatedType* explicitType;
};

class VariableSelfNode : public VariableNodeParser {
public:
  VariableSelfNode()
    : VariableNodeParser("self", NULL) {}

  virtual void registerSymbol(analysis::SemanticContext* ctx);

  virtual ASTNode* rewriteLocal(analysis::SemanticContext* ctx,
                                RewriteMode mode) {
    return NULL;
  }

  virtual void codeGen(backend::CodeGenerator& cg);

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL_EXPR(VariableSelfNode)

protected:
  virtual analysis::InstantiatedType*
    typeCheckImpl(analysis::SemanticContext* ctx,
                  analysis::InstantiatedType* expected,
                  const analysis::InstantiatedTypeVec& typeParamArgs);

public:
  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(self)";
  }
};

class VariableSuperNode : public VariableNodeParser {
public:
  VariableSuperNode()
    : VariableNodeParser("super", NULL) {}

  virtual void registerSymbol(analysis::SemanticContext* ctx);

  virtual ASTNode* rewriteLocal(analysis::SemanticContext* ctx,
                                RewriteMode mode) {
    return NULL;
  }

protected:
  virtual analysis::InstantiatedType*
    typeCheckImpl(analysis::SemanticContext* ctx,
                  analysis::InstantiatedType* expected,
                  const analysis::InstantiatedTypeVec& typeParamArgs);

  virtual void codeGen(backend::CodeGenerator& cg);

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL_EXPR(VariableSuperNode)

public:
  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(super)";
  }
};

}
}

#endif /* VENOM_AST_VARIABLE_H */
