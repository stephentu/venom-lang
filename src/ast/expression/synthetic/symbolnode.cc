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

#include <ast/expression/synthetic/symbolnode.h>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>

#include <backend/bytecode.h>
#include <backend/codegenerator.h>

#include <util/macros.h>

using namespace std;
using namespace venom::analysis;
using namespace venom::backend;

namespace venom {
namespace ast {

SymbolNode::SymbolNode(
    BaseSymbol* symbol,
    const TypeTranslator& translator,
    InstantiatedType* explicitType)
  : VariableNode(symbol->getName()), explicitType(explicitType) {
  assert(symbol);
  this->symbol     = symbol;
  this->translator = translator;
}

void
SymbolNode::codeGen(CodeGenerator& cg) {
  if (ModuleSymbol* msym = dynamic_cast<ModuleSymbol*>(symbol)) {
    // get the class symbol for the module
    ClassSymbol* moduleClassSymbol = msym->getModuleClassSymbol();
    SemanticContext* ctx =
      moduleClassSymbol->getDefinedSymbolTable()->getSemanticContext();

    // find the class reference index for moduleClassSymbol
    bool create;
    size_t classRefIdx =
      cg.enterClass(moduleClassSymbol->getType()->instantiate(ctx), create);
    assert(!create);

    // load the module from the constant pool
    size_t constIdx = cg.createConstant(Constant(classRefIdx), create);

    // push the const onto the stack
    cg.emitInstU32(Instruction::PUSH_CONST, constIdx);
  } else {
    VariableNode::codeGen(cg);
  }
}

void
SymbolNode::print(ostream& o, size_t indent) {
  o << "(symbol-node " << symbol->getFullName();
  if (explicitType) o << " " << explicitType->stringify();
  o << " (sym-addr " << std::hex << symbol << "))";
}

SymbolNode*
SymbolNode::cloneImpl(CloneMode::Type type) {
  assert(type == CloneMode::Semantic);
  return new SymbolNode(symbol, translator, expectedType);
}

ASTExpressionNode*
SymbolNode::cloneForLiftImpl(LiftContext& ctx) {
  // should not clone symbol nodes for lifting...
  VENOM_NOT_REACHED;
}

SymbolNode*
SymbolNode::cloneForTemplateImpl(const TypeTranslator& t) {
  // should not clone symbol nodes for templating...
  VENOM_NOT_REACHED;
}

}
}
