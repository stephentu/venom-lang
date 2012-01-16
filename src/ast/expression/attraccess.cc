#include <ast/expression/attraccess.h>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>

#include <backend/codegenerator.h>

using namespace std;
using namespace venom::analysis;
using namespace venom::backend;

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

void
AttrAccessNode::codeGen(CodeGenerator& cg) {
  primary->codeGen(cg);

  if (hasLocationContext(AssignmentLHS) ||
      hasLocationContext(FunctionCall)) {
    // AssignNode/FunctionCall will take care of the assignment/call
    // respectively
    return;
  }

  // now, we know we are in an rvalue context, so we must produce
  // a result
  BaseSymbol* bs = getSymbol();
  VENOM_ASSERT_TYPEOF_PTR(Symbol, bs);
  Symbol* sym = static_cast<Symbol*>(bs);
  assert(sym->isModuleLevelSymbol() || sym->isObjectField());

  size_t slotIdx = sym->getFieldIndex();
  if (getStaticType()->isRefCounted()) {
    cg.emitInstU32(Instruction::GET_ATTR_OBJ_REF, slotIdx);
  } else {
    cg.emitInstU32(Instruction::GET_ATTR_OBJ, slotIdx);
  }
}

AttrAccessNode*
AttrAccessNode::cloneImpl() {
  return new AttrAccessNode(primary->clone(), name);
}

AttrAccessNode*
AttrAccessNode::cloneForTemplateImpl(const TypeTranslator& t) {
  return new AttrAccessNode(primary->cloneForTemplate(t), name);
}

}
}
