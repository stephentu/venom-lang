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

#include <algorithm>

#include <analysis/semanticcontext.h>
#include <analysis/type.h>

#include <ast/expression/arrayliteral.h>
#include <ast/expression/assignexpr.h>
#include <ast/expression/attraccess.h>
#include <ast/expression/exprlist.h>
#include <ast/expression/functioncall.h>
#include <ast/expression/variable.h>

#include <ast/expression/synthetic/functioncall.h>
#include <ast/expression/synthetic/symbolnode.h>

#include <util/stl.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

ASTNode*
ArrayLiteralNode::rewriteLocal(SemanticContext* ctx, RewriteMode mode) {
  // recurse first
  ASTExpressionNode::rewriteLocal(ctx, mode);
  if (mode != DeSugar) return NULL;

  // now, turn it from:
  // [expr1, expr2, ..., exprN]
  //
  // to:
  // (_tmp = list{type}(), _tmp.append(expr1), ..., _tmp.append(exprN), _tmp)

  InstantiatedType* stype = getStaticType();
  assert(stype->getType()->isListType());
  string tmpVar = ctx->tempVarName();
  ExprNodeVec exprs;
  exprs.reserve(values.size() + 2);
  exprs.push_back(
      new AssignExprNode(
        new VariableNodeParser(tmpVar, NULL),
        new FunctionCallNodeSynthetic(
          new SymbolNode(
            stype->getType()->getClassSymbol()),
          stype->getParams(),
          ExprNodeVec())));
  for (ExprNodeVec::iterator it = values.begin();
       it != values.end(); ++it) {
    exprs.push_back(
        new FunctionCallNodeParser(
          new AttrAccessNode(
            new VariableNodeParser(tmpVar, NULL),
            "append"),
          TypeStringVec(),
          util::vec1((*it)->clone(CloneMode::Semantic))));
  }
  exprs.push_back(new VariableNodeParser(tmpVar, NULL));
  return replace(ctx, new ExprListNode(exprs));
}

struct functor {
  functor(SemanticContext* ctx) : ctx(ctx) {}
  inline InstantiatedType* operator()(ASTExpressionNode* exp) const {
    return exp->typeCheck(ctx);
  }
  SemanticContext* ctx;
};

struct reduce_functor_t {
  inline InstantiatedType*
  operator()(InstantiatedType* accum, InstantiatedType* cur) const {
    return accum->mostCommonType(cur);
  }
} reduce_functor;

InstantiatedType*
ArrayLiteralNode::typeCheckImpl(SemanticContext* ctx,
                                InstantiatedType* expected,
                                const InstantiatedTypeVec& typeParamArgs) {
  if (values.empty()) {
    if (expected && expected->getType()->equals(*Type::ListType)) {
      return expected;
    }
    return Type::ListType->instantiate(
        ctx, util::vec1(InstantiatedType::AnyType));
  } else {
    vector<InstantiatedType*> types(values.size());
    transform(values.begin(), values.end(), types.begin(),
              functor(ctx));
    return Type::ListType->instantiate(
        ctx, util::vec1(
          util::reducel(types.begin(), types.end(), reduce_functor)));
  }
}

ArrayLiteralNode*
ArrayLiteralNode::cloneImpl(CloneMode::Type type) {
  return new ArrayLiteralNode(
      util::transform_vec(
        values.begin(), values.end(),
        ASTExpressionNode::CloneFunctor(type)));
}

ASTExpressionNode*
ArrayLiteralNode::cloneForLiftImpl(LiftContext& ctx) {
  return new ArrayLiteralNode(
      util::transform_vec(
        values.begin(), values.end(),
        ASTExpressionNode::CloneLiftFunctor(ctx)));
}

ArrayLiteralNode*
ArrayLiteralNode::cloneForTemplateImpl(const TypeTranslator& t) {
  return new ArrayLiteralNode(
      util::transform_vec(
        values.begin(), values.end(),
        ASTExpressionNode::CloneTemplateFunctor(t)));
}

}
}
