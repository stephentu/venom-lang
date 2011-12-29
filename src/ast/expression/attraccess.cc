#include <ast/expression/attraccess.h>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

InstantiatedType*
AttrAccessNode::typeCheck(SemanticContext* ctx,
                          InstantiatedType* expected,
                          const InstantiatedTypeVec& typeParamArgs) {
  InstantiatedType *obj = primary->typeCheck(ctx);
  TypeTranslator t;
  BaseSymbol *attrSym =
    obj
      ->getClassSymbolTable()
      ->findBaseSymbol(name, SymbolTable::Any,
                       SymbolTable::AllowCurrentScope, t);
  t.bind(obj);
  if (!attrSym) {
    throw TypeViolationException(
        "Type " + obj->stringify() + " has no member " + name);
  }
  return attrSym->bind(ctx, t, typeParamArgs);
}

}
}
