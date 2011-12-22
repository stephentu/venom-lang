#include <cassert>

#include <ast/node.h>
#include <analysis/symboltable.h>

namespace venom {
namespace ast {

ASTNode::~ASTNode() {
  if (symbols) delete symbols;
}

void ASTNode::initSymbolTable(analysis::SymbolTable* symbols) {
  assert(this->symbols == NULL);
  this->symbols = symbols;

  // set recursively on kids
  for (size_t i = 0; i < getNumKids(); i++) {
    ASTNode *kid = getNthKid(i);
    if (needsNewScope(i)) {
      kid->initSymbolTable(symbols->newChildScope());
    } else {
      kid->initSymbolTable(symbols);
    }
  }
}

}
}
