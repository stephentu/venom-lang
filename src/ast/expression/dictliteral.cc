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

#include <cassert>
#include <utility>

#include <analysis/semanticcontext.h>
#include <analysis/type.h>

#include <ast/expression/assignexpr.h>
#include <ast/expression/attraccess.h>
#include <ast/expression/dictliteral.h>
#include <ast/expression/exprlist.h>
#include <ast/expression/functioncall.h>
#include <ast/expression/variable.h>

#include <ast/expression/synthetic/functioncall.h>
#include <ast/expression/synthetic/symbolnode.h>

#include <backend/codegenerator.h>

#include <util/stl.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

typedef pair<InstantiatedType*, InstantiatedType*> ITypePair;

struct fold_functor {
  fold_functor(SemanticContext* ctx) : ctx(ctx) {}
  inline ITypePair
  operator()(const ITypePair& accum, DictPair* cur) const {
    if (!accum.first) {
      assert(!accum.second);
      return ITypePair(
          cur->key()->typeCheck(ctx),
          cur->value()->typeCheck(ctx));
    } else {
      assert(accum.second);
      return ITypePair(
          accum.first->mostCommonType(cur->key()->typeCheck(ctx)),
          accum.second->mostCommonType(cur->value()->typeCheck(ctx)));
    }
  }
  SemanticContext* ctx;
};

DictPair*
DictPair::cloneImpl(CloneMode::Type type) {
  return new DictPair(first->clone(type), second->clone(type));
}

ASTExpressionNode*
DictPair::cloneForLiftImpl(LiftContext& ctx) {
  return new DictPair(first->cloneForLift(ctx), second->cloneForLift(ctx));
}

DictPair*
DictPair::cloneForTemplateImpl(const TypeTranslator& t) {
  return new DictPair(first->cloneForTemplate(t), second->cloneForTemplate(t));
}

InstantiatedType*
DictLiteralNode::typeCheckImpl(SemanticContext* ctx,
                               InstantiatedType* expected,
                               const InstantiatedTypeVec& typeParamArgs) {
  if (pairs.empty()) {
    if (expected && expected->getType()->equals(*Type::MapType)) {
      return expected;
    }
    return Type::MapType->instantiate(
        ctx, util::vec2(InstantiatedType::AnyType, InstantiatedType::AnyType));
  } else {
    ITypePair res =
      util::foldl(pairs.begin(), pairs.end(),
                  ITypePair(NULL, NULL), fold_functor(ctx));
    return Type::MapType->instantiate(
        ctx, util::vec2(res.first, res.second));
  }
}

ASTNode*
DictLiteralNode::rewriteLocal(SemanticContext* ctx, RewriteMode mode) {
  // recurse first
  ASTExpressionNode::rewriteLocal(ctx, mode);
  if (mode != DeSugar) return NULL;

  // turn it from:
  // { k1 : v1, k2 : v2, ..., kN : vN]
  //
  // to:
  // (_tmp = map{k, v}(), _tmp.set(k1, v1), ..., _tmp.set(kN, vN), _tmp)

  InstantiatedType* stype = getStaticType();
  assert(stype->getType()->isMapType());
  string tmpVar = ctx->tempVarName();
  ExprNodeVec exprs;
  exprs.reserve(pairs.size() + 2);
  exprs.push_back(
      new AssignExprNode(
        new VariableNodeParser(tmpVar, NULL),
        new FunctionCallNodeSynthetic(
          new SymbolNode(
            stype->getType()->getClassSymbol()),
          stype->getParams(),
          ExprNodeVec())));
  for (DictPairVec::iterator it = pairs.begin();
       it != pairs.end(); ++it) {
    exprs.push_back(
        new FunctionCallNodeParser(
          new AttrAccessNode(
            new VariableNodeParser(tmpVar, NULL),
            "set"),
          TypeStringVec(),
          util::vec2((*it)->key()->clone(CloneMode::Semantic),
                     (*it)->value()->clone(CloneMode::Semantic))));
  }
  exprs.push_back(new VariableNodeParser(tmpVar, NULL));
  return replace(ctx, new ExprListNode(exprs));
}

DictLiteralNode*
DictLiteralNode::cloneImpl(CloneMode::Type type) {
  return new DictLiteralNode(
      util::transform_vec(
        pairs.begin(), pairs.end(), DictPair::CloneFunctor(type)));
}

ASTExpressionNode*
DictLiteralNode::cloneForLiftImpl(LiftContext& ctx) {

  ExprNodeVec clonedExprs =
    util::transform_vec(
            pairs.begin(), pairs.end(), DictPair::CloneLiftFunctor(ctx));
  DictPairVec cloned(clonedExprs.size());
  transform(clonedExprs.begin(), clonedExprs.end(), cloned.begin(),
            util::poly_ptr_cast_functor<ASTExpressionNode, DictPair>::checked());
  return new DictLiteralNode(cloned);
}

DictLiteralNode*
DictLiteralNode::cloneForTemplateImpl(const TypeTranslator& t) {
  return new DictLiteralNode(
      util::transform_vec(
        pairs.begin(), pairs.end(), DictPair::CloneTemplateFunctor(t)));
}

}
}
