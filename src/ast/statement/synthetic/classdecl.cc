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

#include <cassert>
#include <algorithm>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>

#include <ast/statement/synthetic/classdecl.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

vector<string>
ClassDeclNodeSynthetic::getTypeParamNames() const {
  vector<string> ret;
  ret.reserve(typeParamTypes.size());
  for (vector<InstantiatedType*>::const_iterator it = typeParamTypes.begin();
       it != typeParamTypes.end(); ++it) {
    ret.push_back((*it)->getType()->getName());
  }
  return ret;
}

void
ClassDeclNodeSynthetic::print(ostream& o, size_t indent) {
  o << "(class " << name << std::endl << util::indent(indent + 1);

  vector<string> parentnames(parentTypes.size());
  transform(parentTypes.begin(), parentTypes.end(),
            parentnames.begin(),
            util::stringify_functor<InstantiatedType>::ptr());

  // parents
  o << "(parents (" <<
    util::join(parentnames.begin(), parentnames.end(), " ") <<
    "))" << std::endl << util::indent(indent + 1);

  vector<string> typenames(typeParamTypes.size());
  transform(typeParamTypes.begin(), typeParamTypes.end(),
            typenames.begin(),
            util::stringify_functor<InstantiatedType>::ptr());

  // type params
  o << "(type-params (" <<
    util::join(typenames.begin(), typenames.end(), ",") <<
    "))" << std::endl << util::indent(indent + 1);

  // statements
  stmts->print(o, indent + 1);

  o << ")";
}

void
ClassDeclNodeSynthetic::checkAndInitTypeParams(SemanticContext* ctx) {
  for (size_t pos = 0; pos < typeParamTypes.size(); pos++) {
    // add all the type params into the body's symtab
    VENOM_ASSERT_TYPEOF_PTR(TypeParamType, typeParamTypes[pos]->getType());
    stmts->getSymbolTable()->createClassSymbol(
        typeParamTypes[pos]->getType()->getName(),
        ctx->getRootSymbolTable()->newChildScopeNoNode(),
        typeParamTypes[pos]->getType());
  }
}

void
ClassDeclNodeSynthetic::checkAndInitParents(SemanticContext* ctx) {}

ClassDeclNode*
ClassDeclNodeSynthetic::cloneImpl(CloneMode::Type type) {
  return new ClassDeclNodeSynthetic(
      name,
      parentTypes,
      typeParamTypes,
      stmts->clone(type));
}

ASTStatementNode*
ClassDeclNodeSynthetic::cloneForLiftImpl(LiftContext& ctx) {
  return cloneForLiftImplHelper(ctx);
}

ClassDeclNode*
ClassDeclNodeSynthetic::cloneForTemplateImpl(const TypeTranslator& t) {
  return cloneForTemplateImplHelper(t);
}

}
}
