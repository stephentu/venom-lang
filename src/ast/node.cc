#include <cassert>

#include <ast/node.h>
#include <analysis/symboltable.h>
#include <util/macros.h>

namespace venom {
namespace ast {

ASTNode::~ASTNode() {
  if (symbols) delete symbols;
}

void ASTNode::initSymbolTable(analysis::SymbolTable* symbols) {
  assert(this->symbols == NULL);
  this->symbols = symbols;

  forchild (kid) {
    if (!kid) continue;
    if (needsNewScope(i)) {
      kid->initSymbolTable(symbols->newChildScope(this));
    } else {
      kid->initSymbolTable(symbols);
    }
  } endfor
}

void
ASTNode::semanticCheckImpl(analysis::SemanticContext* ctx, bool doRegister) {
  if (doRegister) {
    registerSymbol(ctx);
  }
  forchild (kid) {
    if (!kid) continue;
    kid->semanticCheckImpl(ctx, true);
  } endfor
}

}
}
