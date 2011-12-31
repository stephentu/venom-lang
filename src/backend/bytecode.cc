#include <backend/bytecode.h>
#include <backend/vm.h>

using namespace std;
using namespace venom::runtime;

namespace venom {
namespace backend {

bool Instruction::execute(ExecutionContext& ctx) {
  switch (opcode) {

#define HANDLE_ZERO(a) \
    case a:  return a ## _impl(ctx);

#define HANDLE_ONE(a) \
    case a: { \
      venom_cell opnd0 = ctx.program_stack.top(); \
      ctx.program_stack.pop(); \
      return a ## _impl(ctx, opnd0); \
    }

#define HANDLE_TWO(a) \
    case a: { \
      venom_cell opnd0 = ctx.program_stack.top(); \
      ctx.program_stack.pop(); \
      venom_cell opnd1 = ctx.program_stack.top(); \
      ctx.program_stack.pop(); \
      return a ## _impl(ctx, opnd0, opnd1); \
    }

    OPCODE_DEFINER_ZERO(HANDLE_ZERO)
    OPCODE_DEFINER_ONE(HANDLE_ONE)
    OPCODE_DEFINER_TWO(HANDLE_TWO)

  default: break;
  }
  VENOM_NOT_REACHED;
}

template <typename Inst>
static inline Inst* asFormatInst(Instruction* i) {
  assert(i);
  Inst *ii = venom_pointer_cast<Inst*>(i);
  assert(ii);
  return ii;
}

static inline InstFormatA* asFormatA(Instruction* i) {
  return asFormatInst<InstFormatA>(i);
}
static inline InstFormatB* asFormatB(Instruction* i) {
  return asFormatInst<InstFormatB>(i);
}
static inline InstFormatC* asFormatC(Instruction* i) {
  return asFormatInst<InstFormatC>(i);
}

bool Instruction::POP_CELL_impl(ExecutionContext& ctx) {
  ctx.program_stack.pop();
  return true;
}

bool Instruction::PUSH_CELL_impl(ExecutionContext& ctx) {
  InstFormatC *self = asFormatC(this);
  ctx.program_stack.push(self->cell);
  return true;
}

bool Instruction::ALLOC_OBJ_impl(ExecutionContext& ctx) {
  VENOM_UNIMPLEMENTED;
}

bool Instruction::CALL_impl(ExecutionContext& ctx) {
  VENOM_UNIMPLEMENTED;
}

bool Instruction::UNOP_PLUS_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  ctx.program_stack.push(venom_cell(+opnd0.asInt()));
  return true;
}

bool Instruction::UNOP_PLUS_FLOAT_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  ctx.program_stack.push(venom_cell(+opnd0.asDouble()));
  return true;
}

bool Instruction::UNOP_MINUS_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  ctx.program_stack.push(venom_cell(-opnd0.asInt()));
  return true;
}

bool Instruction::UNOP_MINUS_FLOAT_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  ctx.program_stack.push(venom_cell(-opnd0.asDouble()));
  return true;
}

bool Instruction::UNOP_CMP_NOT_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  VENOM_UNIMPLEMENTED;
}

bool Instruction::UNOP_BIT_NOT_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  ctx.program_stack.push(venom_cell(~opnd0.asInt()));
  return true;
}

bool Instruction::BRANCH_Z_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  VENOM_UNIMPLEMENTED;
}

bool Instruction::BRANCH_NZ_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  VENOM_UNIMPLEMENTED;
}

bool Instruction::ATTR_OBJ_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  VENOM_UNIMPLEMENTED;
}

bool Instruction::RET_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  VENOM_UNIMPLEMENTED;
}

bool Instruction::BINOP_ADD_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  ctx.program_stack.push(venom_cell(opnd0.asInt() + opnd1.asInt()));
  return true;
}

bool Instruction::BINOP_ADD_FLOAT_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  ctx.program_stack.push(venom_cell(opnd0.asDouble() + opnd1.asDouble()));
  return true;
}

bool Instruction::BINOP_SUB_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  VENOM_UNIMPLEMENTED;
}

bool Instruction::BINOP_SUB_FLOAT_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  VENOM_UNIMPLEMENTED;
}

bool Instruction::BINOP_MULT_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  VENOM_UNIMPLEMENTED;
}

bool Instruction::BINOP_MULT_FLOAT_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  VENOM_UNIMPLEMENTED;
}

bool Instruction::BINOP_DIV_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  VENOM_UNIMPLEMENTED;
}

bool Instruction::BINOP_DIV_FLOAT_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  VENOM_UNIMPLEMENTED;
}

bool Instruction::BINOP_CMP_AND_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  VENOM_UNIMPLEMENTED;
}

bool Instruction::BINOP_CMP_OR_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  VENOM_UNIMPLEMENTED;
}

bool Instruction::BINOP_CMP_LT_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  VENOM_UNIMPLEMENTED;
}

bool Instruction::BINOP_CMP_LE_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  VENOM_UNIMPLEMENTED;
}

bool Instruction::BINOP_CMP_GT_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  VENOM_UNIMPLEMENTED;
}

bool Instruction::BINOP_CMP_GE_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  VENOM_UNIMPLEMENTED;
}

bool Instruction::BINOP_CMP_EQ_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  VENOM_UNIMPLEMENTED;
}

bool Instruction::BINOP_CMP_NEQ_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  VENOM_UNIMPLEMENTED;
}

bool Instruction::BINOP_BIT_AND_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  VENOM_UNIMPLEMENTED;
}

bool Instruction::BINOP_BIT_OR_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  VENOM_UNIMPLEMENTED;
}

bool Instruction::BINOP_BIT_XOR_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  VENOM_UNIMPLEMENTED;
}

bool Instruction::BINOP_BIT_LSHIFT_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  VENOM_UNIMPLEMENTED;
}

bool Instruction::BINOP_BIT_RSHIFT_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  VENOM_UNIMPLEMENTED;
}

}
}
