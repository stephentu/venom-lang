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
#include <fstream>
#include <iostream>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>

#include <ast/statement/import.h>

#include <parser/driver.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

string
ImportStmtNode::getFileName(SemanticContext* ctx) const {
  return global_compile_opts.venom_import_path + "/" +
         util::join(names.begin(), names.end(), "/") + ".venom";
}

string
ImportStmtNode::getModuleName() const {
  return util::join(names.begin(), names.end(), ".");
}

void
ImportStmtNode::registerSymbol(SemanticContext* ctx) {
  // make sure the name isn't already defined
  // in the current scope
  if (symbols->isDefined(
        names.back(), SymbolTable::Any, SymbolTable::NoRecurse)) {
    throw SemanticViolationException(
        "Symbol " + names.back() + " is already defined in scope");
  }

  // check to see if we already parsed module
  SemanticContext *mctx = ctx->getProgramRoot()->findModule(names);
  if (!mctx) {
    // parse the module file
    string fname(getFileName(ctx));
    fstream file(fname.c_str());
    if (!file.good()) {
      throw SemanticViolationException(
          "No such file " + fname + " to import module " + getModuleName());
    }
    mctx = ctx->getProgramRoot()->createModule(names);
    unsafe_compile_module(fname, file, *mctx);
  }

  assert(mctx);
  assert(mctx->getRootSymbolTable());
  assert(mctx->getModuleRoot());
  assert(mctx->getModuleRoot()->getSymbolTable());

  // find the module symbol (in the module's root scope)
  ModuleSymbol *msym =
    mctx->getRootSymbolTable()->findModuleSymbol(
        names.back(), SymbolTable::NoRecurse);
  assert(msym);

  // create the module symbol in this scope
  // (to make the module visible to this scope)
  symbols->createModuleSymbol(
        names.back(),
        mctx->getModuleRoot()->getSymbolTable(),
        msym->getModuleClassSymbol(),
        ctx);
}

ImportStmtNode*
ImportStmtNode::cloneImpl(CloneMode::Type type) {
  return new ImportStmtNode(names);
}

ASTStatementNode*
ImportStmtNode::cloneForLiftImpl(LiftContext& ctx) {
  return new ImportStmtNode(names);
}

ImportStmtNode*
ImportStmtNode::cloneForTemplateImpl(const TypeTranslator& t) {
  return new ImportStmtNode(names);
}

}
}
