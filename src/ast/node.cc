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
      kid->initSymbolTable(symbols->newChildScope());
    } else {
      kid->initSymbolTable(symbols);
    }
  } endfor
}

void ASTNode::semanticCheck(analysis::SemanticContext* ctx) {
  forchild (kid) {
    if (!kid) continue;
    kid->semanticCheck(ctx);
  } endfor
}

}
}
