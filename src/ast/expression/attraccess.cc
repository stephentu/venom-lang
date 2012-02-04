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

#include <ast/expression/attraccess.h>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>

#include <backend/codegenerator.h>

using namespace std;
using namespace venom::analysis;
using namespace venom::backend;

namespace venom {
namespace ast {

BaseSymbol*
AttrAccessNode::getSymbol() {
  InstantiatedType *obj = primary->getStaticType();
  TypeTranslator t;
  return obj
    ->findCodeGeneratableClassSymbol()
    ->getClassSymbolTable()
    ->findBaseSymbol(
      name, SymbolTable::Any, SymbolTable::ClassLookup, t);
}

InstantiatedType*
AttrAccessNode::typeCheckImpl(SemanticContext* ctx,
                              InstantiatedType* expected,
                              const InstantiatedTypeVec& typeParamArgs) {
  InstantiatedType *obj = primary->typeCheck(ctx);
  TypeTranslator t;
  BaseSymbol *attrSym =
    obj
      ->getClassSymbolTable()
      ->findBaseSymbol(name, SymbolTable::Any,
                       SymbolTable::ClassLookup, t);
  t.bind(obj);
  if (!attrSym) {
    throw TypeViolationException(
        "Type " + obj->stringify() + " has no member " + name);
  }
  return attrSym->bind(ctx, t, typeParamArgs);
}

void
AttrAccessNode::codeGen(CodeGenerator& cg) {
  primary->codeGen(cg);

  if (hasLocationContext(AssignmentLHS) ||
      hasLocationContext(FunctionCall)) {
    // AssignNode/FunctionCall will take care of the assignment/call
    // respectively
    return;
  }

  // now, we know we are in an rvalue context, so we must produce
  // a result
  BaseSymbol* bs = getSymbol();
  VENOM_ASSERT_TYPEOF_PTR(Symbol, bs);
  Symbol* sym = static_cast<Symbol*>(bs);
  assert(sym->isModuleLevelSymbol() || sym->isObjectField());

  size_t slotIdx = sym->getFieldIndex();
  if (getStaticType()->isRefCounted()) {
    cg.emitInstU32(Instruction::GET_ATTR_OBJ_REF, slotIdx);
  } else {
    cg.emitInstU32(Instruction::GET_ATTR_OBJ, slotIdx);
  }
}

AttrAccessNode*
AttrAccessNode::cloneImpl(CloneMode::Type type) {
  return new AttrAccessNode(primary->clone(type), name);
}

ASTExpressionNode*
AttrAccessNode::cloneForLiftImpl(LiftContext& ctx) {
  return new AttrAccessNode(primary->cloneForLift(ctx), name);
}

AttrAccessNode*
AttrAccessNode::cloneForTemplateImpl(const TypeTranslator& t) {
  return new AttrAccessNode(primary->cloneForTemplate(t), name);
}

}
}
