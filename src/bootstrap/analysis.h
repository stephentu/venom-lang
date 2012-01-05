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

backend::Linker::FuncDescMap GetBuiltinFunctionMap();

}
}

#endif /* VENOM_BOOTSTRAP_ANALYSIS_H */
