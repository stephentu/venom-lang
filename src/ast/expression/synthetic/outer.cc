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

#include <ast/expression/synthetic/outer.h>

#include <ast/statement/classdecl.h>

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

void
OuterNode::codeGen(CodeGenerator& cg) {
  primary->codeGen(cg);

  // find the cgen-type
  ClassSymbol* csym =
    primary->getStaticType()->findCodeGeneratableClassSymbol();

  // lookup the <outer> attribute
  TypeTranslator t;
  Symbol* outerSym =
    csym->getClassSymbolTable()->findSymbol(
        "<outer>", SymbolTable::NoRecurse, t);
  assert(outerSym);
  assert(outerSym->isObjectField());

  // code-gen the <outer> attribute
  size_t slotIdx = outerSym->getFieldIndex();
  cg.emitInstU32(Instruction::GET_ATTR_OBJ_REF, slotIdx);
}

OuterNode*
OuterNode::cloneImpl(CloneMode::Type type) {
  return new OuterNode(primary->clone(type));
}

ASTExpressionNode*
OuterNode::cloneForLiftImpl(LiftContext& ctx) {
  return new OuterNode(primary->cloneForLift(ctx));
}

OuterNode*
OuterNode::cloneForTemplateImpl(const TypeTranslator& t) {
  return new OuterNode(primary->cloneForTemplate(t));
}

InstantiatedType*
OuterNode::typeCheckImpl(SemanticContext* ctx,
                         InstantiatedType* expected,
                         const InstantiatedTypeVec& typeParamArgs) {
  InstantiatedType* p = primary->typeCheck(ctx);
  ClassSymbol* cs = p->getClassSymbol();
  ASTNode* owner = cs->getClassSymbolTable()->getOwner();
  if (!owner) {
    throw TypeViolationException(
        "Type " + p->stringify() + " is not a user-defined type");
  }
  VENOM_ASSERT_TYPEOF_PTR(ClassDeclNode, owner);
  ClassDeclNode* cowner = static_cast<ClassDeclNode*>(owner);
  if (!cowner->isNestedClass()) {
    throw TypeViolationException(
        "Type " + p->stringify() + " has no outer class");
  }

  InstantiatedType* outerType =
    cowner->getEnclosingClassNode()->getClassSymbol()->getSelfType(ctx);

  TypeTranslator t;
  t.bind(p);
  return t.translate(ctx, outerType);
}

}
}
