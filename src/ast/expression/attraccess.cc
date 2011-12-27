#include <ast/expression/attraccess.h>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

InstantiatedType*
AttrAccessNode::typeCheck(SemanticContext*  ctx,
                          InstantiatedType* expected) {
  InstantiatedType *obj = primary->typeCheck(ctx, NULL);
  BaseSymbol *attrSym =
    obj
      ->getType()
      ->getClassSymbol()
      ->getClassSymbolTable()
      ->findBaseSymbol(name, SymbolTable::Any, true);
  if (!attrSym) {
    throw TypeViolationException(
        "Type " + obj->stringify() + " has no member " + name);
  }
  return attrSym->bind(ctx, obj->getParams());
}

}
}
