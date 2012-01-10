#include <cassert>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>

#include <ast/statement/whilestmt.h>

#include <backend/bytecode.h>
#include <backend/codegenerator.h>

using namespace std;
using namespace venom::analysis;
using namespace venom::backend;

namespace venom {
namespace ast {

void
WhileStmtNode::codeGen(CodeGenerator& cg) {
  Label *loop = cg.newBoundLabel();
  Label *done = cg.newLabel();
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
  cg.emitInstLabel(op, done);
  stmts->codeGen(cg);
  cg.emitInstLabel(Instruction::JUMP, loop);
  cg.bindLabel(done);
}

WhileStmtNode*
WhileStmtNode::cloneImpl() {
  return new WhileStmtNode(cond->clone(), stmts->clone());
}

}
}
