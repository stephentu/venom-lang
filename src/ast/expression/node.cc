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
#include <sstream>

#include <analysis/semanticcontext.h>

#include <ast/expression/functioncall.h>
#include <ast/expression/node.h>

#include <ast/expression/synthetic/functioncall.h>
#include <ast/expression/synthetic/symbolnode.h>

#include <util/stl.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

string ParameterizedTypeString::stringify() const {
  stringstream s;
  s << util::join(names.begin(), names.end(), ".");
  if (!params.empty()) {
    s << "{";
    vector<string> buf;
    buf.resize(params.size());
    transform(params.begin(), params.end(), buf.begin(), StringerFunctor());
    s << util::join(buf.begin(), buf.end(), ",");
    s << "}";
  }
  return s.str();
}

ParameterizedTypeString* ParameterizedTypeString::clone() {
  return new ParameterizedTypeString(names,
      util::transform_vec(params.begin(), params.end(), CloneFunctor()));
}

void
ASTExpressionNode::collectSpecialized(
    SemanticContext* ctx,
    const TypeTranslator& t,
    CollectCallback& callback) {
  ASTNode::collectSpecialized(ctx, t, callback);
  if (staticType) {
    InstantiatedType* itype = t.translate(ctx, staticType);
    if (itype->isSpecializedType()) callback.offerType(itype);
  }
}

ASTNode*
ASTExpressionNode::rewriteLocal(SemanticContext* ctx,
                                RewriteMode mode) {
  if (mode != BoxPrimitives) return ASTNode::rewriteLocal(ctx, mode);

  // recurse on children
  ASTNode* rep = ASTNode::rewriteLocal(ctx, mode);
  VENOM_ASSERT_NULL(rep);

  // if the expected type is any, and the
  // static type is a primitive, then we need to box
  assert(getStaticType());

  if (getExpectedType() &&
      getExpectedType()->isAny() &&
      getStaticType()->isPrimitive()) {
    ClassSymbol* boxClass = NULL;
    if (getStaticType()->isInt()) {
      boxClass = Type::BoxedIntType->getClassSymbol();
    } else if (getStaticType()->isFloat()) {
      boxClass = Type::BoxedFloatType->getClassSymbol();
    } else if (getStaticType()->isBool()) {
      boxClass = Type::BoxedBoolType->getClassSymbol();
    } else assert(false);
    assert(boxClass);

    FunctionCallNode* rep =
      new FunctionCallNodeSynthetic(
          new SymbolNode(boxClass),
          InstantiatedTypeVec(),
          util::vec1(this->clone(CloneMode::Semantic)));
    return replace(ctx, rep);
  }
  return NULL;
}

ASTExpressionNode*
ASTExpressionNode::replace(SemanticContext* ctx, ASTNode* replacement) {
  assert(replacement);
  VENOM_ASSERT_TYPEOF_PTR(ASTExpressionNode, replacement);
  return static_cast<ASTExpressionNode*>(ASTNode::replace(ctx, replacement));
}

}
}
