#include <ast/statement/classdecl.h>

#include <analysis/symbol.h>
#include <analysis/symboltable.h>

using namespace venom::ast;

namespace venom {
namespace analysis {

bool FuncSymbol::isConstructor() const {
  const ClassDeclNode *cdn =
    dynamic_cast<const ClassDeclNode*>(getSymbolTable()->getOwner());
  if (!cdn) return false;
  return (cdn->getName() == name);
}

bool FuncSymbol::isMethod() const {
  const ClassDeclNode *cdn =
    dynamic_cast<const ClassDeclNode*>(getSymbolTable()->getOwner());
  return cdn;
}

}
}
