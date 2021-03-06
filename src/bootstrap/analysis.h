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

#ifndef VENOM_BOOTSTRAP_ANALYSIS_H
#define VENOM_BOOTSTRAP_ANALYSIS_H

#include <analysis/semanticcontext.h>
#include <analysis/symboltable.h>

#include <backend/linker.h>

namespace venom {
namespace bootstrap {

/**
 * Create a new root bootstrap symbol table.
 * This contains all the builtin types + funcs + classes
 *
 * For now, the implementation hard-codes the builtin
 * information.
 * TODO: implement a more dynamic version
 */
analysis::SymbolTable*
NewBootstrapSymbolTable(analysis::SemanticContext* ctx);

backend::Linker::FuncDescMap
GetBuiltinFunctionMap(analysis::SemanticContext* rootCtx);

backend::Linker::ClassObjMap
GetBuiltinClassMap(analysis::SemanticContext* rootCtx);

}
}

#endif /* VENOM_BOOTSTRAP_ANALYSIS_H */
