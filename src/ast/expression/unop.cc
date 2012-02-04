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

#include <ast/expression/unop.h>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

string UnopNode::StringifyType(Type type) {
  switch (type) {
  case PLUS: return "+";
  case MINUS: return "-";
  case CMP_NOT: return "not";
  case BIT_NOT: return "~";
  default: assert(false);
  }
  return "";
}

InstantiatedType*
UnopNode::typeCheckImpl(SemanticContext* ctx,
                        InstantiatedType* expected,
                        const InstantiatedTypeVec& typeParamArgs) {
  InstantiatedType *kidType = kid->typeCheck(ctx);

  switch (type) {
  case PLUS:
  case MINUS:
    // require numeric
    if (!kidType->isNumeric()) {
      throw TypeViolationException(
          "Expected numeric type for unary operator " +
          StringifyType(type) + ", got " + kidType->stringify());
    }
    return kidType;

  case BIT_NOT:
    // require int
    if (!kidType->isInt()) {
      throw TypeViolationException(
          "Expected type int for unary operator " + StringifyType(type) +
          ", got " + kidType->stringify());
    }
    return kidType;

  case CMP_NOT:
    // allow any type
    return InstantiatedType::BoolType;

  default: assert(false);
  }
  return NULL;
}

UnopNode*
UnopNode::cloneImpl(CloneMode::Type t) {
  return new UnopNode(kid->clone(t), type);
}

ASTExpressionNode*
UnopNode::cloneForLiftImpl(LiftContext& ctx) {
  return new UnopNode(kid->cloneForLift(ctx), type);
}

UnopNode*
UnopNode::cloneForTemplateImpl(const TypeTranslator& t) {
  return new UnopNode(kid->cloneForTemplate(t), type);
}

}
}
