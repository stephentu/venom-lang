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

#include <ast/expression/synthetic/variable.h>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

void
VariableNodeSynthetic::print(ostream& o, size_t indent) {
  o << "(ident " << name;
  o << " " << explicitType->stringify();
	if (symbol) o << " (sym-addr " << std::hex << symbol << ")";
  o << ")";
}

VariableNode*
VariableNodeSynthetic::cloneImpl(CloneMode::Type type) {
  return new VariableNodeSynthetic(name, explicitType);
}

ASTExpressionNode*
VariableNodeSynthetic::cloneForLiftImpl(LiftContext& ctx) {

  // assert that we should never be a non-local ref
#ifndef NDEBUG
  Symbol* s;
  assert(!isNonLocalRef(ctx.definedIn, s));
  BaseSymbol* bs = symbol;
  assert(bs != ctx.curLiftSym);
  LiftContext::LiftMap::const_iterator it =
    ctx.liftMap.find(bs);
  assert(it == ctx.liftMap.end());
#endif /* NDEBUG */

  return new VariableNodeSynthetic(name, explicitType);
}

VariableNode*
VariableNodeSynthetic::cloneForTemplateImpl(const TypeTranslator& t) {
  return new VariableNodeSynthetic(
      name,
      t.translate(getSymbolTable()->getSemanticContext(), explicitType));
}

}
}
