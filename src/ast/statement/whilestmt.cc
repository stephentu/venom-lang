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
  cg.emitInstLabel(Instruction::BRANCH_Z, done);
  stmts->codeGen(cg);
  cg.emitInstLabel(Instruction::JUMP, loop);
  cg.bindLabel(done);
}

}
}
