#include <backend/bytecode.h>
#include <backend/vm.h>
#include <util/macros.h>

using namespace std;
using namespace venom::runtime;

namespace venom {
namespace backend {

bool Instruction::execute(ExecutionContext& ctx) {
  switch (opcode) {

#define HANDLE_ZERO(a) \
    case a: return a ## _impl(ctx);

#define HANDLE_ONE(a) \
    case a: { \
      venom_cell opnd0 = ctx.program_stack.top(); \
      ctx.program_stack.pop(); \
      return a ## _impl(ctx, opnd0); \
    }

#define HANDLE_TWO(a) \
    case a: { \
      venom_cell opnd1 = ctx.program_stack.top(); \
      ctx.program_stack.pop(); \
      venom_cell opnd0 = ctx.program_stack.top(); \
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
static inline Inst* asFormatInst(Instruction* i) { return static_cast<Inst*>(i); }

inline InstFormatU32*
Instruction::asFormatU32() { return asFormatInst<InstFormatU32>(this); }

inline InstFormatI32*
Instruction::asFormatI32() { return asFormatInst<InstFormatI32>(this); }

inline InstFormatIPtr*
Instruction::asFormatIPtr() { return asFormatInst<InstFormatIPtr>(this); }

//inline InstFormatU32U32*
//Instruction::asFormatU32U32() { return asFormatInst<InstFormatU32U32>(this); }

inline InstFormatC*
Instruction::asFormatC() { return asFormatInst<InstFormatC>(this); }

bool Instruction::PUSH_CELL_INT_impl(ExecutionContext& ctx) {
  InstFormatC *self = asFormatC();
  ctx.program_stack.push(venom_cell(self->data.int_value));
  return true;
}

bool Instruction::PUSH_CELL_FLOAT_impl(ExecutionContext& ctx) {
  InstFormatC *self = asFormatC();
  ctx.program_stack.push(venom_cell(self->data.double_value));
  return true;
}

bool Instruction::PUSH_CELL_BOOL_impl(ExecutionContext& ctx) {
  InstFormatC *self = asFormatC();
  ctx.program_stack.push(venom_cell(self->data.bool_value));
  return true;
}

bool Instruction::PUSH_CELL_NIL_impl(ExecutionContext& ctx) {
  ctx.program_stack.push(venom_cell(venom_object::Nil));
  return true;
}

bool Instruction::PUSH_CONST_impl(ExecutionContext& ctx) {
  // TODO: consider storing the pointer in the inst...
  InstFormatU32 *self = asFormatU32();
  venom_cell* konst = ctx.constant_pool[self->N0];
  assert(konst);
  ctx.program_stack.push(*konst);
  return true;
}

bool Instruction::LOAD_LOCAL_VAR_impl(ExecutionContext& ctx) {
  InstFormatU32 *self = asFormatU32();
  ctx.program_stack.push(ctx.local_variables()[self->N0]);
  return true;
}

bool Instruction::ALLOC_OBJ_impl(ExecutionContext& ctx) {
  // TODO: consider storing the pointer in the inst...
  InstFormatU32 *self = asFormatU32();
  venom_class_object* class_obj = ctx.class_obj_pool[self->N0];
  assert(class_obj);

  size_t s =
    venom_object::venom_object_sizeof(
        class_obj->sizeof_obj_base, class_obj->n_cells);

  venom_object *obj = (venom_object *) operator new (s);
  new (obj) venom_object(class_obj);
  ctx.program_stack.push(venom_cell(obj));
  return true;
}

bool Instruction::CALL_impl(ExecutionContext& ctx) {
  InstFormatIPtr *self = asFormatIPtr();
  // push ret address
  ctx.program_stack.push(venom_cell(int64_t(ctx.program_counter + 1)));
  // create new local variable frame
  ctx.new_frame();
  // set PC
  ctx.program_counter = reinterpret_cast<Instruction**>(self->N0);
  return false;
}

bool Instruction::CALL_NATIVE_impl(ExecutionContext& ctx) {
  InstFormatIPtr *self = asFormatIPtr();
  FunctionDescriptor *desc = reinterpret_cast<FunctionDescriptor*>(self->N0);
  assert(desc->isNative());
  desc->dispatch(&ctx);
  return true;
}

bool Instruction::JUMP_impl(ExecutionContext& ctx) {
  InstFormatI32 *self = asFormatI32();
  // set PC
  ctx.program_counter = ctx.program_counter + 1 + self->N0;
  return false;
}

bool Instruction::POP_CELL_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  return true;
}

bool Instruction::STORE_LOCAL_VAR_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  InstFormatU32 *self = asFormatU32();
  vector<venom_cell> &vars = ctx.local_variables();
  if (self->N0 >= vars.size()) vars.resize(self->N0 + 1);
  vars[self->N0] = opnd0;
  return true;
}

bool Instruction::INT_TO_FLOAT_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  ctx.program_stack.push(venom_cell(double(opnd0.asInt())));
  return true;
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
  ctx.program_stack.push(venom_cell(opnd0.falseTest()));
  return true;
}

bool Instruction::UNOP_BIT_NOT_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  ctx.program_stack.push(venom_cell(~opnd0.asInt()));
  return true;
}

bool Instruction::BRANCH_Z_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  if (!opnd0.truthTest()) {
    InstFormatI32 *self = asFormatI32();
    ctx.program_counter = ctx.program_counter + 1 + self->N0;
    return false;
  }
  return true;
}

bool Instruction::BRANCH_NZ_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  if (opnd0.truthTest()) {
    InstFormatI32 *self = asFormatI32();
    ctx.program_counter = ctx.program_counter + 1 + self->N0;
    return false;
  }
  return true;
}

bool Instruction::TEST_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  ctx.program_stack.push(opnd0.truthTest());
  return true;
}

bool Instruction::GET_ATTR_OBJ_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  InstFormatU32 *self = asFormatU32();
  ctx.program_stack.push(opnd0.asRawObject()->cell(self->N0));
  return true;
}

bool Instruction::GET_ARRAY_ACCESS_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  VENOM_UNIMPLEMENTED;
}

bool Instruction::DUP_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  InstFormatU32 *self = asFormatU32();
  for (size_t i = 0; i < self->N0 + 1; i++) {
    ctx.program_stack.push(venom_cell(opnd0));
  }
  return true;
}

bool Instruction::CALL_VIRTUAL_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  InstFormatU32 *self = asFormatU32();
  FunctionDescriptor *desc =
    opnd0.asRawObject()->getClassObj()->vtable.at(self->N0);
  assert(desc);

  // push "this" pointer
  ctx.program_stack.push(opnd0);

  if (desc->isNative()) {
    // use native dispatch
    desc->dispatch(&ctx);
    return true;
  } else {
    // push ret address
    ctx.program_stack.push(venom_cell(int64_t(ctx.program_counter + 1)));
    // create new local variable frame
    ctx.new_frame();
    // set PC
    ctx.program_counter =
      reinterpret_cast<Instruction**>(desc->getFunctionPtr());
    return false;
  }
}

#define IMPL_BINOP0(transform, op) \
  ctx.program_stack.push(venom_cell(opnd0 transform op opnd1 transform))
#define IMPL_BINOP(op)       IMPL_BINOP0(, op)
#define IMPL_BINOP_INT(op)   IMPL_BINOP0(.asInt(), op)
#define IMPL_BINOP_FLOAT(op) IMPL_BINOP0(.asDouble(), op)
#define IMPL_BINOP_TRUTH(op) IMPL_BINOP0(.truthTest(), op)

bool Instruction::BINOP_ADD_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  IMPL_BINOP_INT(+);
  return true;
}

bool Instruction::BINOP_ADD_FLOAT_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  IMPL_BINOP_FLOAT(+);
  return true;
}

bool Instruction::BINOP_SUB_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  IMPL_BINOP_INT(-);
  return true;
}

bool Instruction::BINOP_SUB_FLOAT_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  IMPL_BINOP_FLOAT(-);
  return true;
}

bool Instruction::BINOP_MULT_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  IMPL_BINOP_INT(*);
  return true;
}

bool Instruction::BINOP_MULT_FLOAT_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  IMPL_BINOP_FLOAT(*);
  return true;
}

bool Instruction::BINOP_DIV_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  IMPL_BINOP_INT(/);
  return true;
}

bool Instruction::BINOP_DIV_FLOAT_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  IMPL_BINOP_FLOAT(/);
  return true;
}

bool Instruction::BINOP_MOD_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  IMPL_BINOP_INT(%);
  return true;
}

bool Instruction::BINOP_CMP_AND_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  IMPL_BINOP_TRUTH(&&);
  return true;
}

bool Instruction::BINOP_CMP_OR_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  IMPL_BINOP_TRUTH(||);
  return true;
}

bool Instruction::BINOP_CMP_LT_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  IMPL_BINOP(<);
  return true;
}

bool Instruction::BINOP_CMP_LE_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  IMPL_BINOP(<=);
  return true;
}

bool Instruction::BINOP_CMP_GT_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  IMPL_BINOP(>);
  return true;
}

bool Instruction::BINOP_CMP_GE_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  IMPL_BINOP(>=);
  return true;
}

bool Instruction::BINOP_CMP_EQ_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  IMPL_BINOP(==);
  return true;
}

bool Instruction::BINOP_CMP_NEQ_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  IMPL_BINOP(!=);
  return true;
}

bool Instruction::BINOP_BIT_AND_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  IMPL_BINOP_INT(&);
  return true;
}

bool Instruction::BINOP_BIT_OR_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  IMPL_BINOP_INT(|);
  return true;
}

bool Instruction::BINOP_BIT_XOR_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  IMPL_BINOP_INT(^);
  return true;
}

bool Instruction::BINOP_BIT_LSHIFT_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  IMPL_BINOP_INT(<<);
  return true;
}

bool Instruction::BINOP_BIT_RSHIFT_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  IMPL_BINOP_INT(>>);
  return true;
}

#undef IMPL_BINOP0
#undef IMPL_BINOP
#undef IMPL_BINOP_INT
#undef IMPL_BINOP_FLOAT
#undef IMPL_BINOP_TRUTH

bool Instruction::SET_ATTR_OBJ_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  InstFormatU32 *self = asFormatU32();
  opnd0.asRawObject()->cell(self->N0) = opnd1;
  return true;
}

bool Instruction::SET_ARRAY_ACCESS_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  VENOM_UNIMPLEMENTED;
}

bool Instruction::RET_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  ctx.program_counter = reinterpret_cast<Instruction**>(opnd0.asInt());
  ctx.program_stack.push(opnd1);
  ctx.pop_frame();
  return false;
}

}
}
