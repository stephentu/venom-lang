/**
 * Copyright (c) 2012 Stephen Tu
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names
 * of its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <backend/bytecode.h>
#include <backend/vm.h>
#include <runtime/venomlist.h>
#include <util/macros.h>

using namespace std;
using namespace venom::runtime;

namespace venom {
namespace backend {

static inline void CheckNullPointer(const venom_cell& cell) {
  if (VENOM_UNLIKELY(!cell.asRawObject())) {
    throw VenomRuntimeException("Null pointer dereferenced");
  }
}

// Don't be tempted to rewrite execute to use function pointers-
// I tried it and it runs a bit slower

/*
typedef bool(Instruction::*FARG0)(ExecutionContext&);
typedef bool(Instruction::*FARG1)(ExecutionContext&,venom_cell&);
typedef bool(Instruction::*FARG2)(ExecutionContext&,venom_cell&,venom_cell&);

bool Instruction::execute(ExecutionContext& ctx) {

#define OP_HANDLER(a) &Instruction::a ## _impl,
  static FARG0 FArg0Ptrs[] = {
    OPCODE_DEFINER_ZERO(OP_HANDLER)
  };
  static FARG1 FArg1Ptrs[] = {
    OPCODE_DEFINER_ONE(OP_HANDLER)
  };
  static FARG2 FArg2Ptrs[] = {
    OPCODE_DEFINER_TWO(OP_HANDLER)
  };
#undef OP_HANDLER

  size_t code = static_cast<size_t>(opcode);
  if (code < VENOM_NELEMS(FArg0Ptrs)) {
    // no args
    FARG0 fcn = FArg0Ptrs[code];
    return (this->*fcn)(ctx);
  } else if (code < (VENOM_NELEMS(FArg0Ptrs) + VENOM_NELEMS(FArg1Ptrs))) {
    // one arg
    FARG1 fcn = FArg1Ptrs[opcode - VENOM_NELEMS(FArg0Ptrs)];
    stack<venom_cell>& pstack = ctx.program_stack;
    venom_cell opnd0 = pstack.top();
    pstack.pop();
    return (this->*fcn)(ctx, opnd0);
  } else {
    // two args
    assert(
        code <
        (VENOM_NELEMS(FArg0Ptrs) +
         VENOM_NELEMS(FArg1Ptrs) +
         VENOM_NELEMS(FArg2Ptrs)));
    FARG2 fcn =
      FArg2Ptrs[code - (VENOM_NELEMS(FArg0Ptrs) + VENOM_NELEMS(FArg1Ptrs))];
    stack<venom_cell>& pstack = ctx.program_stack;
    venom_cell opnd1 = pstack.top();
    pstack.pop();
    venom_cell opnd0 = pstack.top();
    pstack.pop();
    return (this->*fcn)(ctx, opnd0, opnd1);
  }
}
*/

bool Instruction::execute(ExecutionContext& ctx) {
  VENOM_TRACE(stringify(opcode));

  switch (opcode) {

#define HANDLE_ZERO(a) \
    case a: return a ## _impl(ctx);

#define HANDLE_ONE(a) \
    case a: { \
      ExecutionContext::program_stack_type& pstack = ctx.program_stack; \
      venom_cell opnd0 = pstack.top(); \
      pstack.pop(); \
      return a ## _impl(ctx, opnd0); \
    }

#define HANDLE_TWO(a) \
    case a: { \
      ExecutionContext::program_stack_type& pstack = ctx.program_stack; \
      venom_cell opnd1 = pstack.top(); \
      pstack.pop(); \
      venom_cell opnd0 = pstack.top(); \
      pstack.pop(); \
      return a ## _impl(ctx, opnd0, opnd1); \
    }

#define HANDLE_THREE(a) \
    case a: { \
      ExecutionContext::program_stack_type& pstack = ctx.program_stack; \
      venom_cell opnd2 = pstack.top(); \
      pstack.pop(); \
      venom_cell opnd1 = pstack.top(); \
      pstack.pop(); \
      venom_cell opnd0 = pstack.top(); \
      pstack.pop(); \
      return a ## _impl(ctx, opnd0, opnd1, opnd2); \
    }

    OPCODE_DEFINER_ZERO(HANDLE_ZERO)
    OPCODE_DEFINER_ONE(HANDLE_ONE)
    OPCODE_DEFINER_TWO(HANDLE_TWO)
    OPCODE_DEFINER_THREE(HANDLE_THREE)

#undef HANDLE_ZERO
#undef HANDLE_ONE
#undef HANDLE_TWO
#undef HANDLE_THREE

  default: assert(false);
  }
  return false;
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
  konst->incRef();
  ctx.program_stack.push(*konst);
  return true;
}

bool Instruction::LOAD_LOCAL_VAR_impl(ExecutionContext& ctx) {
  InstFormatU32 *self = asFormatU32();
  ctx.program_stack.push(ctx.local_variable(self->N0));
  return true;
}

bool Instruction::LOAD_LOCAL_VAR_REF_impl(ExecutionContext& ctx) {
  InstFormatU32 *self = asFormatU32();
  venom_cell &cell = ctx.local_variable(self->N0);
  venom_cell::AssertNonZeroRefCount(cell);
  cell.incRef();
  ctx.program_stack.push(cell);
  return true;
}

bool Instruction::ALLOC_OBJ_impl(ExecutionContext& ctx) {
  InstFormatIPtr *self = asFormatIPtr();
  venom_class_object* class_obj =
    reinterpret_cast<venom_class_object*>(self->N0);
  venom_object* obj = venom_object::allocObj(class_obj);
  obj->incRef();
  ctx.program_stack.push(venom_cell(obj));
  return true;
}

bool Instruction::CALL_impl(ExecutionContext& ctx) {
  InstFormatIPtr *self = asFormatIPtr();
  // create new local variable frame
  ctx.new_frame(ctx.program_counter + 1);
  // set PC
  FunctionDescriptor *desc = reinterpret_cast<FunctionDescriptor*>(self->N0);
  ctx.program_counter =
    ctx.code->instructions.begin() + int64_t(desc->getFunctionPtr());
  return false;
}

bool Instruction::CALL_NATIVE_impl(ExecutionContext& ctx) {
  InstFormatIPtr *self = asFormatIPtr();
  FunctionDescriptor *desc = reinterpret_cast<FunctionDescriptor*>(self->N0);
  assert(desc->isNative());
  desc->dispatch(&ctx);
  return true;
}

bool Instruction::RET_impl(ExecutionContext& ctx) {
  Instruction** ret_addr = ctx.pop_frame();
  ctx.program_counter = ret_addr;
  return false;
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

bool Instruction::POP_CELL_REF_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  venom_cell::AssertNonZeroRefCount(opnd0);
  opnd0.decRef();
  return true;
}

bool Instruction::STORE_LOCAL_VAR_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  assert(ctx.local_variables_stack.size() ==
         ctx.local_variables_ref_info_stack.size());

  InstFormatU32 *self = asFormatU32();
  size_t last_offset = ctx.frame_offset.top();
  if (VENOM_UNLIKELY(last_offset + self->N0 >=
                     ctx.local_variables_stack.size())) {
    ctx.local_variables_stack.resize(
        last_offset + self->N0 + 1);
    ctx.local_variables_ref_info_stack.resize(
        last_offset + self->N0 + 1, false);
  }

  assert(!ctx.local_variable_ref_info(self->N0));
  ctx.local_variable(self->N0) = opnd0;
  return true;
}

bool Instruction::STORE_LOCAL_VAR_REF_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  venom_cell::AssertNonZeroRefCount(opnd0);
  assert(ctx.local_variables_stack.size() ==
         ctx.local_variables_ref_info_stack.size());

  InstFormatU32 *self = asFormatU32();
  size_t last_offset = ctx.frame_offset.top();
  if (VENOM_UNLIKELY(last_offset + self->N0 >=
                     ctx.local_variables_stack.size())) {
    ctx.local_variables_stack.resize(
        last_offset + self->N0 + 1);
    ctx.local_variables_ref_info_stack.resize(
        last_offset + self->N0 + 1, false);
  }

  venom_cell& old = ctx.local_variable(self->N0);
  if (ctx.local_variable_ref_info(self->N0)) {
#ifndef NDEBUG
    if (old.asRawObject() == opnd0.asRawObject()) {
      // self assignment should *not* be a problem
      assert(!opnd0.asRawObject() || opnd0.asRawObject()->getCount() > 1);
    }
#endif
    old.decRef();
  }
  old = opnd0;
  ctx.local_variables_ref_info_stack[last_offset + self->N0] = true;
  return true;
}

bool Instruction::INT_TO_FLOAT_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  ctx.program_stack.push(venom_cell(double(opnd0.asInt())));
  return true;
}

bool Instruction::FLOAT_TO_INT_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  ctx.program_stack.push(venom_cell(int64_t(opnd0.asDouble())));
  return true;
}

#define IMPL_UNOP_1(transform, op) \
  ctx.program_stack.push(venom_cell(op(opnd0 transform)))
#define IMPL_UNOP_2(transform, op0, op1) \
  ctx.program_stack.push(venom_cell(op1(op0(opnd0 transform))))

#define IMPL_UNOP_INT_1(op)   IMPL_UNOP_1(.asInt(),       op)
#define IMPL_UNOP_FLOAT_1(op) IMPL_UNOP_1(.asDouble(),    op)
#define IMPL_UNOP_BOOL_1(op)  IMPL_UNOP_1(.asBool(),      op)
#define IMPL_UNOP_REF_1(op) \
  do { \
    venom_cell::AssertNonZeroRefCount(opnd0); \
    IMPL_UNOP_1(.asRawObject(), op); \
    opnd0.decRef(); \
  } while (0);

#define IMPL_UNOP_INT_2(op0, op1)   IMPL_UNOP_2(.asInt(),    op0, op1)
#define IMPL_UNOP_FLOAT_2(op0, op1) IMPL_UNOP_2(.asDouble(), op0, op1)
#define IMPL_UNOP_BOOL_2(op0, op1)  IMPL_UNOP_2(.asBool(),   op0, op1)
#define IMPL_UNOP_REF_2(op0, op1) \
  do { \
    venom_cell::AssertNonZeroRefCount(opnd0); \
    IMPL_UNOP_2(.asRawObject(), op0, op1); \
    opnd0.decRef(); \
  } while (0);

bool Instruction::UNOP_PLUS_INT_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  IMPL_UNOP_INT_1(+);
  return true;
}

bool Instruction::UNOP_PLUS_FLOAT_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  IMPL_UNOP_FLOAT_1(+);
  return true;
}

bool Instruction::UNOP_MINUS_INT_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  IMPL_UNOP_INT_1(-);
  return true;
}

bool Instruction::UNOP_MINUS_FLOAT_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  IMPL_UNOP_FLOAT_1(-);
  return true;
}

bool Instruction::UNOP_CMP_NOT_INT_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  IMPL_UNOP_INT_2(!, bool);
  return true;
}

bool Instruction::UNOP_CMP_NOT_FLOAT_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  IMPL_UNOP_FLOAT_2(!, bool);
  return true;
}

bool Instruction::UNOP_CMP_NOT_BOOL_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  IMPL_UNOP_BOOL_2(!, bool);
  return true;
}

bool Instruction::UNOP_CMP_NOT_REF_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  IMPL_UNOP_REF_2(!, bool);
  return true;
}

bool Instruction::UNOP_BIT_NOT_INT_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  IMPL_UNOP_INT_1(~);
  return true;
}

#define IMPL_BRANCH(transform, op, action) \
  do { \
    if (op(opnd0.as ## transform ())) { \
      InstFormatI32 *self = asFormatI32(); \
      ctx.program_counter = ctx.program_counter + 1 + self->N0; \
      action; \
      return false; \
    } \
    action; \
    return true; \
  } while (0)

#define IMPL_BRANCH_Z(transform)  IMPL_BRANCH(transform, !, )
#define IMPL_BRANCH_NZ(transform) IMPL_BRANCH(transform,  , )

#define IMPL_BRANCH_Z_ACT(transform, act)  IMPL_BRANCH(transform, !, act)
#define IMPL_BRANCH_NZ_ACT(transform, act) IMPL_BRANCH(transform,  , act)

bool Instruction::BRANCH_Z_INT_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  IMPL_BRANCH_Z(Int);
}

bool Instruction::BRANCH_Z_FLOAT_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  IMPL_BRANCH_Z(Double);
}

bool Instruction::BRANCH_Z_BOOL_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  IMPL_BRANCH_Z(Bool);
}

bool Instruction::BRANCH_Z_REF_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  venom_cell::AssertNonZeroRefCount(opnd0);
  IMPL_BRANCH_Z_ACT(RawObject, opnd0.decRef());
}

bool Instruction::BRANCH_NZ_INT_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  IMPL_BRANCH_NZ(Int);
}

bool Instruction::BRANCH_NZ_FLOAT_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  IMPL_BRANCH_NZ(Double);
}

bool Instruction::BRANCH_NZ_BOOL_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  IMPL_BRANCH_NZ(Bool);
}

bool Instruction::BRANCH_NZ_REF_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  venom_cell::AssertNonZeroRefCount(opnd0);
  IMPL_BRANCH_NZ_ACT(RawObject, opnd0.decRef());
}

#undef IMPL_BRANCH
#undef IMPL_BRANCH_Z
#undef IMPL_BRANCH_NZ
#undef IMPL_BRANCH_Z_ACT
#undef IMPL_BRANCH_NZ_ACT

bool Instruction::TEST_INT_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  IMPL_UNOP_INT_1(bool);
  return true;
}

bool Instruction::TEST_FLOAT_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  IMPL_UNOP_FLOAT_1(bool);
  return true;
}

bool Instruction::TEST_REF_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  IMPL_UNOP_REF_1(bool);
  return true;
}

bool Instruction::GET_ATTR_OBJ_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  CheckNullPointer(opnd0);
  venom_cell::AssertNonZeroRefCount(opnd0);
  InstFormatU32 *self = asFormatU32();

  ctx.program_stack.push(opnd0.asRawObject()->cell(self->N0));

  opnd0.decRef();
  return true;
}

bool Instruction::GET_ATTR_OBJ_REF_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  CheckNullPointer(opnd0);
  venom_cell::AssertNonZeroRefCount(opnd0);
  InstFormatU32 *self = asFormatU32();

  venom_cell& cell = opnd0.asRawObject()->cell(self->N0);
  venom_cell::AssertNonZeroRefCount(cell);
  cell.incRef();
  ctx.program_stack.push(cell);

  opnd0.decRef();
  return true;
}

bool Instruction::DUP_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  InstFormatU32 *self = asFormatU32();
  for (size_t i = 0; i < self->N0 + 1; i++) {
    ctx.program_stack.push(opnd0);
  }
  return true;
}

bool Instruction::DUP_REF_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  venom_cell::AssertNonZeroRefCount(opnd0);
  InstFormatU32 *self = asFormatU32();
  for (size_t i = 0; i < self->N0 + 1; i++) {
    ctx.program_stack.push(opnd0);
    if (i < self->N0) opnd0.incRef();
  }
  return true;
}

bool Instruction::CALL_VIRTUAL_impl(ExecutionContext& ctx, venom_cell& opnd0) {
  CheckNullPointer(opnd0);
  venom_cell::AssertNonZeroRefCount(opnd0);
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
    // create new local variable frame
    ctx.new_frame(ctx.program_counter + 1);
    // set PC
    ctx.program_counter =
      ctx.code->instructions.begin() + int64_t(desc->getFunctionPtr());
    return false;
  }
}

#undef IMPL_UNOP_1
#undef IMPL_UNOP_2
#undef IMPL_UNOP_INT_1
#undef IMPL_UNOP_FLOAT_1
#undef IMPL_UNOP_BOOL_1
#undef IMPL_UNOP_REF_1

#define IMPL_BINOP0(transform, op) \
  ctx.program_stack.push(venom_cell(opnd0 transform op opnd1 transform))
#define IMPL_BINOP_INT(op)   IMPL_BINOP0(.asInt(),       op)
#define IMPL_BINOP_FLOAT(op) IMPL_BINOP0(.asDouble(),    op)
#define IMPL_BINOP_BOOL(op)  IMPL_BINOP0(.asBool(),      op)
#define IMPL_BINOP_REF(op) \
  do { \
    venom_cell::AssertNonZeroRefCount(opnd0); \
    venom_cell::AssertNonZeroRefCount(opnd1); \
    IMPL_BINOP0(.asRawObject(), op); \
    opnd0.decRef(); \
    opnd1.decRef(); \
  } while (0)

bool Instruction::BINOP_ADD_INT_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  IMPL_BINOP_INT(+);
  return true;
}

bool Instruction::BINOP_ADD_FLOAT_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  IMPL_BINOP_FLOAT(+);
  return true;
}

bool Instruction::BINOP_SUB_INT_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  IMPL_BINOP_INT(-);
  return true;
}

bool Instruction::BINOP_SUB_FLOAT_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  IMPL_BINOP_FLOAT(-);
  return true;
}

bool Instruction::BINOP_MULT_INT_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  IMPL_BINOP_INT(*);
  return true;
}

bool Instruction::BINOP_MULT_FLOAT_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  IMPL_BINOP_FLOAT(*);
  return true;
}

bool Instruction::BINOP_DIV_INT_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  IMPL_BINOP_INT(/);
  return true;
}

bool Instruction::BINOP_DIV_FLOAT_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  IMPL_BINOP_FLOAT(/);
  return true;
}

bool Instruction::BINOP_MOD_INT_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  IMPL_BINOP_INT(%);
  return true;
}

#define IMPL_BINOP_CMP(x) \
  x(INT) \
  x(FLOAT) \
  x(BOOL) \
  x(REF) \

#define IMPL_BINOP_RELATIONAL(x) \
  x(INT) \
  x(FLOAT) \
  x(BOOL) \

#define IMPL_BINOP_BIT(x) \
  x(INT) \
  x(BOOL) \

#define OP_BINOP(type, fname, name, op) \
  bool Instruction::BINOP_##fname##_##name##_##type##_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) { \
    IMPL_BINOP_##type(op); \
    return true; \
  } \

#define OP_CMP_AND(type) OP_BINOP(type, CMP, AND, &&)
#define OP_CMP_OR(type)  OP_BINOP(type, CMP, OR, ||)
#define OP_CMP_LT(type)  OP_BINOP(type, CMP, LT, <)
#define OP_CMP_LE(type)  OP_BINOP(type, CMP, LE, <=)
#define OP_CMP_GT(type)  OP_BINOP(type, CMP, GT, >)
#define OP_CMP_GE(type)  OP_BINOP(type, CMP, GE, >=)
#define OP_CMP_EQ(type)  OP_BINOP(type, CMP, EQ, ==)
#define OP_CMP_NEQ(type) OP_BINOP(type, CMP, NEQ, !=)

#define OP_BIT_AND(type) OP_BINOP(type, BIT, AND, &)
#define OP_BIT_OR(type)  OP_BINOP(type, BIT, OR, |)
#define OP_BIT_XOR(type) OP_BINOP(type, BIT, XOR, ^)

IMPL_BINOP_CMP(OP_CMP_AND)
IMPL_BINOP_CMP(OP_CMP_OR)

IMPL_BINOP_RELATIONAL(OP_CMP_LT)
IMPL_BINOP_RELATIONAL(OP_CMP_LE)
IMPL_BINOP_RELATIONAL(OP_CMP_GT)
IMPL_BINOP_RELATIONAL(OP_CMP_GE)

IMPL_BINOP_CMP(OP_CMP_EQ)
IMPL_BINOP_CMP(OP_CMP_NEQ)

IMPL_BINOP_BIT(OP_BIT_AND)
IMPL_BINOP_BIT(OP_BIT_OR)
IMPL_BINOP_BIT(OP_BIT_XOR)

#undef IMPL_BINOP_CMP
#undef IMPL_BINOP_RELATIONAL
#undef IMPL_BINOP_BIT

#undef OP_BINOP
#undef OP_CMP_AND
#undef OP_CMP_OR
#undef OP_CMP_LT
#undef OP_CMP_LE
#undef OP_CMP_GT
#undef OP_CMP_GE
#undef OP_CMP_EQ
#undef OP_CMP_NEQ
#undef OP_BIT_AND
#undef OP_BIT_OR
#undef OP_BIT_XOR

bool Instruction::BINOP_BIT_LSHIFT_INT_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  IMPL_BINOP_INT(<<);
  return true;
}

bool Instruction::BINOP_BIT_RSHIFT_INT_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  IMPL_BINOP_INT(>>);
  return true;
}

#undef IMPL_BINOP0
#undef IMPL_BINOP_INT
#undef IMPL_BINOP_FLOAT
#undef IMPL_BINOP_BOOL
#undef IMPL_BINOP_REF

bool Instruction::SET_ATTR_OBJ_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  CheckNullPointer(opnd0);
  venom_cell::AssertNonZeroRefCount(opnd0);
  InstFormatU32 *self = asFormatU32();
  // make sure the obj's cell is actually meant for primitives
  assert(!(opnd0.asRawObject()->getClassObj()->ref_cell_bitmap
        & (0x1 << self->N0)));
  opnd0.asRawObject()->cell(self->N0) = opnd1;
  opnd0.decRef();
  return true;
}

bool Instruction::SET_ATTR_OBJ_REF_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  CheckNullPointer(opnd0);
  venom_cell::AssertNonZeroRefCount(opnd0);
  venom_cell::AssertNonZeroRefCount(opnd1);
  InstFormatU32 *self = asFormatU32();
  // make sure the obj's cell is actually meant for references
  assert(opnd0.asRawObject()->getClassObj()->ref_cell_bitmap
        & (0x1 << self->N0));
  venom_cell& old = opnd0.asRawObject()->cell(self->N0);
#ifndef NDEBUG
  if (old.asRawObject() == opnd1.asRawObject()) {
    // self assignment should *not* be a problem
    assert(!opnd1.asRawObject() || opnd1.asRawObject()->getCount() > 1);
  }
#endif
  old.decRef();
  old = opnd1;
  opnd0.decRef();
  return true;
}

bool Instruction::GET_ARRAY_ACCESS_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  CheckNullPointer(opnd0);
  venom_cell::AssertNonZeroRefCount(opnd0);

  // directly access the array instead of calling the virtual method get()

  // compile time assert that all the primitive list types are the same
  // size + have the same offset for elems. this way, we can treat all
  // primitive arrays the same

  VENOM_COMPILE_TIME_ASSERT(
    sizeof(venom_list::int_list_type) ==
    sizeof(venom_list::float_list_type));
  VENOM_COMPILE_TIME_ASSERT(
    sizeof(venom_list::float_list_type) ==
    sizeof(venom_list::bool_list_type));

  // WARNING: offsetof() is technically not allowed for non-POD types, but
  // should be ok on most compilers since venom_list_impl is non-virtual
  VENOM_COMPILE_TIME_ASSERT(
    offsetof(venom_list::int_list_type, elems) ==
    offsetof(venom_list::float_list_type, elems));
  VENOM_COMPILE_TIME_ASSERT(
    offsetof(venom_list::float_list_type, elems) ==
    offsetof(venom_list::bool_list_type, elems));

  // this is OK b/c of the compile-time checks
  venom_list::int_list_type* l =
    static_cast<venom_list::int_list_type*>(opnd0.asRawObject());
  ctx.program_stack.push(l->elems.at(opnd1.asInt()));

  opnd0.decRef();
  return true;
}

bool Instruction::GET_ARRAY_ACCESS_REF_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1) {
  CheckNullPointer(opnd0);
  venom_cell::AssertNonZeroRefCount(opnd0);

  // directly access the array instead of calling the virtual method get()
  venom_list::ref_list_type* l =
    static_cast<venom_list::ref_list_type*>(opnd0.asRawObject());
  venom_cell cell = l->elems.at(opnd1.asInt());
  venom_cell::AssertNonZeroRefCount(cell);
  cell.incRef();
  ctx.program_stack.push(cell);

  opnd0.decRef();
  return true;
}

bool Instruction::SET_ARRAY_ACCESS_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1, venom_cell& opnd2) {
  CheckNullPointer(opnd0);
  venom_cell::AssertNonZeroRefCount(opnd0);
  // directly access the array instead of calling the virtual method set()

  // see the compile time asserts in GET_ARRAY_ACCESS_impl()
  venom_list::int_list_type* l =
    static_cast<venom_list::int_list_type*>(opnd0.asRawObject());
  l->elems.at(opnd1.asInt()) = opnd2;
  opnd0.decRef();
  return true;
}

bool Instruction::SET_ARRAY_ACCESS_REF_impl(ExecutionContext& ctx, venom_cell& opnd0, venom_cell& opnd1, venom_cell& opnd2) {
  CheckNullPointer(opnd0);
  venom_cell::AssertNonZeroRefCount(opnd0);
  venom_cell::AssertNonZeroRefCount(opnd2);
  // directly access the array instead of calling the virtual method set()
  venom_list::ref_list_type* l =
    static_cast<venom_list::ref_list_type*>(opnd0.asRawObject());
  venom_cell &old = l->elems.at(opnd1.asInt());
  old.decRef();
  old = opnd2;
  opnd0.decRef();
  return true;
}

}
}
