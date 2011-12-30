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
ImportStmtNode::getFileName() const {
  return global_compile_opts.venom_import_path + "/" +
         util::join(names.begin(), names.end(), "/") + ".venom";
}

string
ImportStmtNode::getModuleName() const {
  return util::join(names.begin(), names.end(), ".");
}

void
ImportStmtNode::registerSymbol(SemanticContext* ctx) {
  // check to see if we already parsed module
  SemanticContext *mctx = ctx->getProgramRoot()->findModule(names);
  if (mctx) {
    // simply add the module symbol to this scope's symbol table

  } else {
    // parse the module file
    string fname(getFileName());
    fstream file(fname.c_str());
    if (!file.good()) {
      throw SemanticViolationException(
          "No such file " + fname + " to import module " + getModuleName());
    }
    SemanticContext *newCtx = ctx->getProgramRoot()->createModule(names);
    unsafe_compile(fname, file, *newCtx);
  }
}

}
}
