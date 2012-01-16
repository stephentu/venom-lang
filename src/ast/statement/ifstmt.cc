#include <cassert>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>

#include <ast/statement/ifstmt.h>

#include <backend/bytecode.h>
#include <backend/codegenerator.h>

using namespace std;
using namespace venom::analysis;
using namespace venom::backend;

namespace venom {
namespace ast {

void IfStmtNode::typeCheck(SemanticContext* ctx, InstantiatedType* expected) {
  cond->typeCheck(ctx);
  true_branch->typeCheck(ctx, expected);
  false_branch->typeCheck(ctx, expected);
}

ASTNode*
IfStmtNode::rewriteReturn(SemanticContext* ctx) {
  ASTNode *trueRep = true_branch->rewriteReturn(ctx);
  VENOM_ASSERT_NULL(trueRep);
  ASTNode *falseRep = false_branch->rewriteReturn(ctx);
  VENOM_ASSERT_NULL(falseRep);
  return NULL;
}

void IfStmtNode::codeGen(CodeGenerator& cg) {
  Label *done = cg.newLabel();
  Label *falseBranch = cg.newLabel();

  cond->codeGen(cg);
  Instruction::Opcode op;
  if (cond->getStaticType()->isInt()) {
    op = Instruction::BRANCH_Z_INT;
  } else if (cond->getStaticType()->isFloat()) {
    op = Instruction::BRANCH_Z_FLOAT;
  } else if (cond->getStaticType()->isBool()) {
    op = Instruction::BRANCH_Z_BOOL;
  } else {
    op = Instruction::BRANCH_Z_REF;
  }
  cg.emitInstLabel(op, falseBranch);
  true_branch->codeGen(cg);
  cg.emitInstLabel(Instruction::JUMP, done);
  cg.bindLabel(falseBranch);
  false_branch->codeGen(cg);
  cg.bindLabel(done);
}

IfStmtNode*
IfStmtNode::cloneImpl() {
  return new IfStmtNode(
      cond->clone(), true_branch->clone(), false_branch->clone());
}

IfStmtNode*
IfStmtNode::cloneForTemplateImpl(const TypeTranslator& t) {
  return new IfStmtNode(
      cond->cloneForTemplate(t),
      true_branch->cloneForTemplate(t),
      false_branch->cloneForTemplate(t));
}

}
}
