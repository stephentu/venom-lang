#include <ast/expression/attraccess.h>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

BaseSymbol*
AttrAccessNode::getSymbol() {
  InstantiatedType *obj = primary->getStaticType();
  TypeTranslator t;
  return obj->getClassSymbolTable()->findBaseSymbol(
      name, SymbolTable::Any, SymbolTable::ClassLookup, t);
}

InstantiatedType*
AttrAccessNode::typeCheckImpl(SemanticContext* ctx,
                              InstantiatedType* expected,
                              const InstantiatedTypeVec& typeParamArgs) {
  InstantiatedType *obj = primary->typeCheck(ctx);
  TypeTranslator t;
  BaseSymbol *attrSym =
    obj
      ->getClassSymbolTable()
      ->findBaseSymbol(name, SymbolTable::Any,
                       SymbolTable::ClassLookup, t);
  t.bind(obj);
  if (!attrSym) {
    throw TypeViolationException(
        "Type " + obj->stringify() + " has no member " + name);
  }
  return attrSym->bind(ctx, t, typeParamArgs);
}

AttrAccessNode*
AttrAccessNode::cloneImpl() {
  return new AttrAccessNode(primary->clone(), name);
}

}
}
