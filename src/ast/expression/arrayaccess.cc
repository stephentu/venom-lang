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

#include <analysis/semanticcontext.h>
#include <analysis/type.h>

#include <ast/expression/arrayaccess.h>

#include <backend/codegenerator.h>

using namespace std;
using namespace venom::analysis;
using namespace venom::backend;

namespace venom {
namespace ast {

InstantiatedType*
ArrayAccessNode::typeCheckImpl(SemanticContext* ctx,
                               InstantiatedType* expected,
                               const InstantiatedTypeVec& typeParamArgs) {

  InstantiatedType *primaryType = primary->typeCheck(ctx);
  InstantiatedType *indexType = index->typeCheck(ctx);

  bool isList = primaryType->getType()->equals(*Type::ListType);
  bool isMap  = primaryType->getType()->equals(*Type::MapType);
  bool isStr  = primaryType->getType()->equals(*Type::StringType);

  if (!isList && !isMap && !isStr) {
    throw TypeViolationException(
        "Cannot subscript non-list/non-map/non-string type: " +
        primaryType->stringify());
  }

  if (isList || isStr) {
    if (!indexType->equals(*InstantiatedType::IntType)) {
      throw TypeViolationException(
          "Invalid index type - expecting int, got " +
          indexType->stringify());
    }
    return isList ? primaryType->getParams().at(0) : primaryType;
  } else {
    InstantiatedType *keyType = primaryType->getParams().at(0);
    if (!indexType->equals(*keyType)) {
      throw TypeViolationException(
          "Invalid index type - expecting " + keyType->stringify() +
          ", got " + indexType->stringify());
    }
    return primaryType->getParams().at(1);
  }
}

void
ArrayAccessNode::codeGen(CodeGenerator& cg) {
  assert(!hasLocationContext(AssignmentLHS));
  if (primary->getStaticType()->getType()->isListType()) {
    assert(index->getStaticType()->isInt());
    primary->codeGen(cg);
    index->codeGen(cg);
    cg.emitInst(
        primary->getStaticType()->getParams()[0]->isRefCounted() ?
          Instruction::GET_ARRAY_ACCESS_REF:
          Instruction::GET_ARRAY_ACCESS);
  } else VENOM_UNIMPLEMENTED;
}

void
ArrayAccessNode::codeGenAssignLHS(CodeGenerator& cg, ASTExpressionNode* value) {
  assert(hasLocationContext(AssignmentLHS));
  if (primary->getStaticType()->getType()->isListType()) {
    assert(index->getStaticType()->isInt());
    assert(value->getStaticType()->isSubtypeOf(
          *(primary->getStaticType()->getParams()[0])));
    primary->codeGen(cg);
    index->codeGen(cg);
    value->codeGen(cg);
    cg.emitInst(
        primary->getStaticType()->getParams()[0]->isRefCounted() ?
          Instruction::SET_ARRAY_ACCESS_REF:
          Instruction::SET_ARRAY_ACCESS);
  } else VENOM_UNIMPLEMENTED;
}

ArrayAccessNode*
ArrayAccessNode::cloneImpl(CloneMode::Type type) {
  return new ArrayAccessNode(primary->clone(type), index->clone(type));
}

ASTExpressionNode*
ArrayAccessNode::cloneForLiftImpl(LiftContext& ctx) {
  return new ArrayAccessNode(primary->cloneForLift(ctx), index->cloneForLift(ctx));
}

ArrayAccessNode*
ArrayAccessNode::cloneForTemplateImpl(const TypeTranslator& t) {
  return new ArrayAccessNode(
      primary->cloneForTemplate(t), index->cloneForTemplate(t));
}

}
}
