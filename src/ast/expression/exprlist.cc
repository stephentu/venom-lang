#include <ast/expression/exprlist.h>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>

#include <backend/bytecode.h>
#include <backend/codegenerator.h>

#include <util/macros.h>

using namespace std;
using namespace venom::analysis;
using namespace venom::backend;

namespace venom {
namespace ast {

void
ExprListNode::codeGen(CodeGenerator& cg) {
  for (ExprNodeVec::iterator it = exprs.begin();
       it != exprs.end(); ++it) {
    if (it + 1 != exprs.end()) {
      // regular
      (*it)->codeGen(cg);
      // pop off the stack
      cg.emitInst((*it)->getStaticType()->isRefCounted() ?
          Instruction::POP_CELL_REF : Instruction::POP_CELL);
    } else {
      // last
      (*it)->codeGen(cg);
      // don't pop off the stack
    }
  }
}

InstantiatedType*
ExprListNode::typeCheckImpl(SemanticContext* ctx,
                            InstantiatedType* expected,
                            const InstantiatedTypeVec& typeParamArgs) {
  InstantiatedType* ret = NULL;
  for (ExprNodeVec::iterator it = exprs.begin();
       it != exprs.end(); ++it) {
    if (it + 1 != exprs.end()) {
      // regular
      (*it)->typeCheck(ctx, NULL, InstantiatedTypeVec());
    } else {
      // last
      ret = (*it)->typeCheck(ctx, expected, typeParams);
    }
  }
  assert(ret);
  return ret;
}

ExprListNode*
ExprListNode::cloneImpl() {
  return new ExprListNode(
      util::transform_vec(
        exprs.begin(), exprs.end(),
        ASTExpressionNode::CloneFunctor()));
}

ASTExpressionNode*
ExprListNode::cloneForLiftImpl(LiftContext& ctx) {
  return new ExprListNode(
      util::transform_vec(
        exprs.begin(), exprs.end(),
        ASTExpressionNode::CloneLiftFunctor(ctx)));
}

ExprListNode*
ExprListNode::cloneForTemplateImpl(const TypeTranslator& t) {
  return new ExprListNode(
      util::transform_vec(
        exprs.begin(), exprs.end(),
        ASTExpressionNode::CloneTemplateFunctor(t)));
}

}
}
