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

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>

#include <ast/statement/synthetic/funcdecl.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

FuncDeclNode*
FuncDeclNodeSynthetic::cloneImpl(CloneMode::Type type) {
  return new FuncDeclNodeSynthetic(
    name,
    typeParamTypes,
    util::transform_vec(params.begin(), params.end(),
      ASTExpressionNode::CloneFunctor(type)),
    retType,
    stmts->clone(type));
}

ASTStatementNode*
FuncDeclNodeSynthetic::cloneForLiftImpl(LiftContext& ctx) {
  return cloneForLiftImplHelper(ctx);
}

FuncDeclNode*
FuncDeclNodeSynthetic::cloneForTemplateImpl(const TypeTranslator& t) {
  return cloneForTemplateImplHelper(t);
}

void
FuncDeclNodeSynthetic::print(ostream& o, size_t indent) {
  o << "(func " << name << " -> " << retType->stringify();
  o << std::endl << util::indent(indent + 1);
  o << "(params ";
  PrintExprNodeVec(o, params, indent + 1);
  o << ")";
  o << std::endl << util::indent(indent + 1);
  stmts->print(o, indent + 1);
  o << ")";
}

void
FuncDeclNodeSynthetic::checkAndInitTypeParams(SemanticContext* ctx) {
  for (size_t pos = 0; pos < typeParamTypes.size(); pos++) {
    // add all the type params into the body's symtab
    VENOM_ASSERT_TYPEOF_PTR(TypeParamType, typeParamTypes[pos]->getType());
    stmts->getSymbolTable()->createClassSymbol(
        typeParamTypes[pos]->getType()->getName(),
        ctx->getRootSymbolTable()->newChildScope(NULL),
        typeParamTypes[pos]->getType());
  }
}

void
FuncDeclNodeSynthetic::checkAndInitReturnType(SemanticContext* ctx) {}

}
}
