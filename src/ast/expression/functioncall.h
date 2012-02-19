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
                   const ExprNodeVec&   args)
    : primary(primary), args(args) {
    primary->addLocationContext(ASTNode::FunctionCall);
  }

  ~FunctionCallNode() {
    delete primary;
    util::delete_pointers(args.begin(), args.end());
  }

  inline ASTExpressionNode* getPrimary() { return primary; }
  inline const ASTExpressionNode* getPrimary() const { return primary; }

  // must call checkAndInitTypeParams() at least once before calling
  virtual std::vector<analysis::InstantiatedType*> getTypeParams() const = 0;

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
    case 0:  VENOM_SAFE_SET_EXPR(primary, kid); break;
    default: VENOM_SAFE_SET_EXPR(args[idx - 1], kid); break;
    }
  }

  virtual bool needsNewScope(size_t k) const {
    VENOM_CHECK_RANGE(k, 1 + args.size());
    return false;
  }

  virtual void registerSymbol(analysis::SemanticContext* ctx) {
    checkAndInitTypeParams(ctx);
  }

  virtual void collectSpecialized(
      analysis::SemanticContext* ctx,
      const analysis::TypeTranslator& t,
      CollectCallback& callback);

private:
  analysis::BaseSymbol* extractSymbol(analysis::BaseSymbol* orig);

public:

  virtual ASTNode* rewriteAfterLift(
      const LiftContext::LiftMap& liftMap,
      const std::set<analysis::BaseSymbol*>& refs);

  VENOM_AST_TYPED_CLONE_EXPR(FunctionCallNode)

protected:
  virtual analysis::InstantiatedType*
    typeCheckImpl(analysis::SemanticContext* ctx,
                  analysis::InstantiatedType* expected,
                  const analysis::InstantiatedTypeVec& typeParamArgs);

public:
  virtual void codeGen(backend::CodeGenerator& cg);

protected:
  virtual void checkAndInitTypeParams(analysis::SemanticContext* ctx) = 0;

  ASTExpressionNode* cloneForLiftImplHelper(LiftContext& ctx);

  ASTExpressionNode* primary;
  ExprNodeVec        args;
};

class FunctionCallNodeParser : public FunctionCallNode {
public:
  /** Takes ownership of typeArgs */
  FunctionCallNodeParser(ASTExpressionNode*   primary,
                         const TypeStringVec& typeArgs,
                         const ExprNodeVec&   args)
    : FunctionCallNode(primary, args), typeArgs(typeArgs) {}

  ~FunctionCallNodeParser() {
    util::delete_pointers(typeArgs.begin(), typeArgs.end());
  }

  virtual std::vector<analysis::InstantiatedType*> getTypeParams() const
    { assert(typeArgs.size() == typeArgTypes.size());
      return typeArgTypes; }

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL_EXPR(FunctionCallNode)

  virtual void print(std::ostream& o, size_t indent = 0);

protected:
  virtual void checkAndInitTypeParams(analysis::SemanticContext* ctx);

private:
  TypeStringVec typeArgs;
  std::vector<analysis::InstantiatedType*> typeArgTypes;
};

}
}

#endif /* VENOM_AST_FUNCTIONCALL_H */
