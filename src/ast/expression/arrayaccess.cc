#include <analysis/semanticcontext.h>
#include <analysis/type.h>

#include <ast/expression/arrayaccess.h>

#include <backend/codegenerator.h>

using namespace std;
using namespace venom::analysis;
using namespace venom::backend;

namespace venom {
namespace ast {

InstantiatedType*
ArrayAccessNode::typeCheckImpl(SemanticContext* ctx,
                               InstantiatedType* expected,
                               const InstantiatedTypeVec& typeParamArgs) {

  InstantiatedType *primaryType = primary->typeCheck(ctx);
  InstantiatedType *indexType = index->typeCheck(ctx);

  bool isList = primaryType->getType()->equals(*Type::ListType);
  bool isMap  = primaryType->getType()->equals(*Type::MapType);
  bool isStr  = primaryType->getType()->equals(*Type::StringType);

  if (!isList && !isMap && !isStr) {
    throw TypeViolationException(
        "Cannot subscript non-list/non-map/non-string type: " +
        primaryType->stringify());
  }

  if (isList || isStr) {
    if (!indexType->equals(*InstantiatedType::IntType)) {
      throw TypeViolationException(
          "Invalid index type - expecting int, got " +
          indexType->stringify());
    }
    return isList ? primaryType->getParams().at(0) : primaryType;
  } else {
    InstantiatedType *keyType = primaryType->getParams().at(0);
    if (!indexType->equals(*keyType)) {
      throw TypeViolationException(
          "Invalid index type - expecting " + keyType->stringify() +
          ", got " + indexType->stringify());
    }
    return primaryType->getParams().at(1);
  }
}

void
ArrayAccessNode::codeGen(CodeGenerator& cg) {
  assert(!hasLocationContext(AssignmentLHS));
  if (primary->getStaticType()->getType()->isListType()) {
    assert(index->getStaticType()->isInt());
    primary->codeGen(cg);
    index->codeGen(cg);
    cg.emitInst(
        primary->getStaticType()->getParams()[0]->isRefCounted() ?
          Instruction::GET_ARRAY_ACCESS_REF:
          Instruction::GET_ARRAY_ACCESS);
  } else VENOM_UNIMPLEMENTED;
}

void
ArrayAccessNode::codeGenAssignLHS(CodeGenerator& cg, ASTExpressionNode* value) {
  assert(hasLocationContext(AssignmentLHS));
  if (primary->getStaticType()->getType()->isListType()) {
    assert(index->getStaticType()->isInt());
    assert(value->getStaticType()->isSubtypeOf(
          *(primary->getStaticType()->getParams()[0])));
    primary->codeGen(cg);
    index->codeGen(cg);
    value->codeGen(cg);
    cg.emitInst(
        primary->getStaticType()->getParams()[0]->isRefCounted() ?
          Instruction::SET_ARRAY_ACCESS_REF:
          Instruction::SET_ARRAY_ACCESS);
  } else VENOM_UNIMPLEMENTED;
}

ArrayAccessNode*
ArrayAccessNode::cloneImpl(CloneMode::Type type) {
  return new ArrayAccessNode(primary->clone(type), index->clone(type));
}

ASTExpressionNode*
ArrayAccessNode::cloneForLiftImpl(LiftContext& ctx) {
  return new ArrayAccessNode(primary->cloneForLift(ctx), index->cloneForLift(ctx));
}

ArrayAccessNode*
ArrayAccessNode::cloneForTemplateImpl(const TypeTranslator& t) {
  return new ArrayAccessNode(
      primary->cloneForTemplate(t), index->cloneForTemplate(t));
}

}
}
