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
 * 3. Neither the name of the PostgreSQL Global Development Group nor the names
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

#ifndef VENOM_BACKEND_BYTECODE_H
#define VENOM_BACKEND_BYTECODE_H

#include <cassert>
#include <string>

#include <runtime/venomobject.h>
#include <util/macros.h>

namespace venom {
namespace backend {

/** Forward decl */
class ExecutionContext;
class InstFormatU32;
class InstFormatI32;
class InstFormatIPtr;
//class InstFormatU32U32;
class InstFormatC;

/**
 * Instruction is the actual executable instruction in the venom vm.
 * A sequence of instructions forms an executable venom program.
 *
 * Note that the CodeGenerator does not generate Instruction instances
 * directly- rather, an Instruction stream is generated after the linking
 * phase.  The reason for this is mainly a time/space efficiency concern;
 * Instruction instances are more compact than their symbolic counterparts and
 * have all references resolved (avoiding un-necessary runtime symbol lookups).
 */
class Instruction {
public:

  /**
   * Venom Bytecode Opcodes
   *
   * Desc format is: [stack before] -> [stack after] ; [side effects]
   *
   * In [stack before], the right hand side represents the top of the stack
   *
   * In the handler functions, the operands are passed in left to right
   * as specified in [stack before]. For example, for BINOP_ADD, the
   * description is:
   *
   *   opnd0, opnd1 -> opnd0 + opnd1
   *
   * Both opnd1 and opnd0 are popped off the stack (in that order), opnd1 is at
   * the top of the stack before the invocation, and opnd0 + opnd1 is pushed
   * onto the stack. The BINOP_ADD_INT handler will be invoked as:
   *
   *   BINOP_ADD_INT_impl(opnd0, opnd1)
   *
   * Zero operand instructions:
   *
   *   PUSH_CELL_INT i64
   *     -> i64
   *   PUSH_CELL_FLOAT double
   *     -> double
   *   PUSH_CELL_BOOL bool
   *     -> bool
   *   PUSH_CELL_NIL
   *     -> Nil
   *   PUSH_CONST N0
   *     -> const[N0] ; incRef(const[N0])
   *
   *   LOAD_LOCAL_VAR N0
   *      -> variables[N0]
   *   LOAD_LOCAL_VAR_REF N0
   *      -> variables[N0] ; incRef(variables[N0])
   *
   *   ALLOC_OBJ N0
   *      -> obj ; incRef(obj)
   *
   *   CALL N0
   *      -> ; pc = N0
   *   CALL_NATIVE N0
   *      [aN, aN-1, ..., a1, a0] -> ret_value ; N0(a0, a1, ..., aN-1, aN)
   *
   *   RET
   *      -> ; pc = ret_addr
   *
   *   JUMP N0
   *      -> ; pc = next_pc + N0
   *
   * One operand instructions:
   *
   *   POP_CELL
   *     opnd0 ->
   *   POP_CELL_REF
   *     opnd0 -> ; decRef(opnd0)
   *
   *   STORE_LOCAL_VAR N0
   *     opnd0 -> ; variables[N0] = opnd0
   *   STORE_LOCAL_VAR_REF N0
   *     opnd0 -> ; decRef(variables[N0]), variables[N0] = opnd0
   *
   *   INT_TO_FLOAT
   *     opnd0 -> float(opnd0)
   *   FLOAT_TO_INT
   *     opnd0 -> int(opnd0)
   *
   *   UNOP_PLUS_INT
   *     opnd0 -> +opnd0
   *   UNOP_PLUS_FLOAT
   *     opnd0 -> +opnd0
   *
   *   UNOP_MINUS_INT
   *     opnd0 -> -opnd0
   *   UNOP_MINUS_FLOAT
   *     opnd0 -> -opnd0
   *
   *   UNOP_CMP_NOT_INT
   *     opnd0 -> !opnd0
   *   UNOP_CMP_NOT_FLOAT
   *     opnd0 -> !opnd0
   *   UNOP_CMP_NOT_BOOL
   *     opnd0 -> !opnd0
   *   UNOP_CMP_NOT_REF
   *     opnd0 -> !opnd0 ; decRef(opnd0)
   *
   *   UNOP_BIT_NOT_INT
   *     opnd0 -> ~opnd0
   *
   *   BRANCH_Z_INT N0
   *     opnd0 -> ; if (opnd0) pc = next_pc + N0 else pc = next_pc
   *   BRANCH_Z_FLOAT N0
   *     opnd0 -> ; if (opnd0) pc = next_pc + N0 else pc = next_pc
   *   BRANCH_Z_BOOL N0
   *     opnd0 -> ; if (opnd0) pc = next_pc + N0 else pc = next_pc
   *   BRANCH_Z_REF N0
   *     opnd0 -> ; if (opnd0) pc = next_pc + N0 else pc = next_pc, decRef(opnd0)
   *
   *   BRANCH_NZ_INT N0
   *     opnd0 -> ; if (!opnd0) pc = next_pc + N0 else pc = next_pc
   *   BRANCH_NZ_FLOAT N0
   *     opnd0 -> ; if (!opnd0) pc = next_pc + N0 else pc = next_pc
   *   BRANCH_NZ_BOOL N0
   *     opnd0 -> ; if (!opnd0) pc = next_pc + N0 else pc = next_pc
   *   BRANCH_NZ_REF N0
   *     opnd0 -> ; if (!opnd0) pc = next_pc + N0 else pc = next_pc, decRef(opnd0)
   *
   *   TEST_INT
   *     opnd0 -> bool(opnd0)
   *   TEST_FLOAT
   *     opnd0 -> bool(opnd0)
   *   TEST_REF
   *     opnd0 -> bool(opnd0) ; decRef(opnd0)
   *
   *   GET_ATTR_OBJ N0
   *     opnd0 -> opnd0.attr[N0] ; decRef(opnd0)
   *   GET_ATTR_OBJ_REF N0
   *     opnd0 -> opnd0.attr[N0] ; incRef(opnd0.attr[N0]), decRef(opnd0)
   *
   *   DUP N0
   *     opnd0 -> opnd0, opnd0, ..., opnd0 (N0 + 1 instances)
   *   DUP_REF N0
   *     opnd0 -> opnd0, opnd0, ..., opnd0 (N0 + 1 instances) ; incRef(opnd0) (N0 times)
   *
   *   CALL_VIRTUAL N0
   *     obj -> ret_value ; PC = obj.vtable[N0] ; decRef(obj)
   *
   * Two operand instructions:
   *
   *   BINOP_ADD_INT
   *     opnd0, opnd1 -> opnd0 + opnd1
   *   BINOP_ADD_FLOAT
   *     opnd0, opnd1 -> opnd0 + opnd1
   *
   *   BINOP_SUB_INT
   *     opnd0, opnd1 -> opnd0 - opnd1
   *   BINOP_SUB_FLOAT
   *     opnd0, opnd1 -> opnd0 - opnd1
   *
   *   BINOP_MULT_INT
   *     opnd0, opnd1 -> opnd0 * opnd1
   *   BINOP_MULT_FLOAT
   *     opnd0, opnd1 -> opnd0 * opnd1
   *
   *   BINOP_DIV_INT
   *     opnd0, opnd1 -> opnd0 / opnd1
   *   BINOP_DIV_FLOAT
   *     opnd0, opnd1 -> opnd0 / opnd1
   *
   *   BINOP_MOD_INT
   *     opnd0, opnd1 -> opnd0 & opnd1
   *
   *   BINOP_CMP_AND_INT
   *     opnd0, opnd1 -> opnd0 && opnd1
   *   BINOP_CMP_AND_FLOAT
   *     opnd0, opnd1 -> opnd0 && opnd1
   *   BINOP_CMP_AND_BOOL
   *     opnd0, opnd1 -> opnd0 && opnd1
   *   BINOP_CMP_AND_REF
   *     opnd0, opnd1 -> opnd0 && opnd1 ; decRef(opnd0), decRef(opnd1)
   *
   *   BINOP_CMP_OR_INT
   *     opnd0, opnd1 -> opnd0 || opnd1
   *   BINOP_CMP_OR_FLOAT
   *     opnd0, opnd1 -> opnd0 || opnd1
   *   BINOP_CMP_OR_BOOL
   *     opnd0, opnd1 -> opnd0 || opnd1
   *   BINOP_CMP_OR_REF
   *     opnd0, opnd1 -> opnd0 || opnd1 ; decRef(opnd0), decRef(opnd1)
   *
   *   BINOP_CMP_LT_INT
   *     opnd0, opnd1 -> opnd0 < opnd1
   *   BINOP_CMP_LT_FLOAT
   *     opnd0, opnd1 -> opnd0 < opnd1
   *   BINOP_CMP_LT_BOOL
   *     opnd0, opnd1 -> opnd0 < opnd1
   *
   *   BINOP_CMP_LE_INT
   *     opnd0, opnd1 -> opnd0 <= opnd1
   *   BINOP_CMP_LE_FLOAT
   *     opnd0, opnd1 -> opnd0 <= opnd1
   *   BINOP_CMP_LE_BOOL
   *     opnd0, opnd1 -> opnd0 <= opnd1
   *
   *   BINOP_CMP_GT_INT
   *     opnd0, opnd1 -> opnd0 > opnd1
   *   BINOP_CMP_GT_FLOAT
   *     opnd0, opnd1 -> opnd0 > opnd1
   *   BINOP_CMP_GT_BOOL
   *     opnd0, opnd1 -> opnd0 > opnd1
   *
   *   BINOP_CMP_GE_INT
   *     opnd0, opnd1 -> opnd0 >= opnd1
   *   BINOP_CMP_GE_FLOAT
   *     opnd0, opnd1 -> opnd0 >= opnd1
   *   BINOP_CMP_GE_BOOL
   *     opnd0, opnd1 -> opnd0 >= opnd1
   *
   *   BINOP_CMP_EQ_INT
   *     opnd0, opnd1 -> opnd0 == opnd1
   *   BINOP_CMP_EQ_FLOAT
   *     opnd0, opnd1 -> opnd0 == opnd1
   *   BINOP_CMP_EQ_BOOL
   *     opnd0, opnd1 -> opnd0 == opnd1
   *   BINOP_CMP_EQ_REF
   *     opnd0, opnd1 -> opnd0 == opnd1 ; decRef(opnd0), decRef(opnd1)
   *
   *   BINOP_CMP_NEQ_INT
   *     opnd0, opnd1 -> opnd0 != opnd1
   *   BINOP_CMP_NEQ_FLOAT
   *     opnd0, opnd1 -> opnd0 != opnd1
   *   BINOP_CMP_NEQ_BOOL
   *     opnd0, opnd1 -> opnd0 != opnd1
   *   BINOP_CMP_NEQ_REF
   *     opnd0, opnd1 -> opnd0 != opnd1 ; decRef(opnd0), decRef(opnd1)
   *
   *   BINOP_BIT_AND_INT
   *     opnd0, opnd1 -> opnd0 & opnd1
   *   BINOP_BIT_AND_BOOL
   *     opnd0, opnd1 -> opnd0 & opnd1
   *
   *   BINOP_BIT_OR_INT
   *     opnd0, opnd1 -> opnd0 | opnd1
   *   BINOP_BIT_OR_BOOL
   *     opnd0, opnd1 -> opnd0 | opnd1
   *
   *   BINOP_BIT_XOR_INT
   *     opnd0, opnd1 -> opnd0 ^ opnd1
   *   BINOP_BIT_XOR_BOOL
   *     opnd0, opnd1 -> opnd0 ^ opnd1
   *
   *   BINOP_BIT_LSHIFT_INT
   *     opnd0, opnd1 -> opnd0 << opnd1
   *
   *   BINOP_BIT_RSHIFT_INT
   *     opnd0, opnd1 -> opnd0 >> opnd1
   *
   *   SET_ATTR_OBJ N0
   *     opnd0, opnd1 -> ; opnd0.attr[N0] = opnd1, decRef(opnd0)
   *   SET_ATTR_OBJ_REF N0
   *     opnd0, opnd1 -> ; decRef(opnd0.attr[N0]),
   *                       opnd0.attr[N0] = opnd1, decRef(opnd0)
   *
   *   GET_ARRAY_ACCESS
   *     opnd0, opnd1 -> opnd0[opnd1] ; decRef(opnd0)
   *   GET_ARRAY_ACCESS_REF
   *     opnd0, opnd1 -> opnd0[opnd1] ; incRef(opnd0[opnd1]), decRef(opnd0)
   *
   * Three operand instructions:
   *
   *   SET_ARRAY_ACCESS
   *     opnd0, opnd1, opnd2 ->
   *        ; opnd0[opnd1] = opnd2,  decRef(opnd0)
   *   SET_ARRAY_ACCESS_REF
   *     opnd0, opnd1, opnd2 ->
   *        ; incRef(opnd2), decRef(opnd0[opnd1]),
   *          opnd0[opnd1] = opnd2, decRef(opnd0)
   *
   */

#define OPCODE_DEFINER_ZERO(x) \
    x(PUSH_CELL_INT) \
    x(PUSH_CELL_FLOAT) \
    x(PUSH_CELL_BOOL) \
    x(PUSH_CELL_NIL) \
    x(PUSH_CONST) \
    x(LOAD_LOCAL_VAR) \
    x(LOAD_LOCAL_VAR_REF) \
    x(ALLOC_OBJ) \
    x(CALL) \
    x(CALL_NATIVE) \
    x(RET) \
    x(JUMP) \

#define OPCODE_DEFINER_ONE(x) \
    x(POP_CELL) \
    x(POP_CELL_REF) \
    x(STORE_LOCAL_VAR) \
    x(STORE_LOCAL_VAR_REF) \
    x(INT_TO_FLOAT) \
    x(FLOAT_TO_INT) \
    x(UNOP_PLUS_INT) \
    x(UNOP_PLUS_FLOAT) \
    x(UNOP_MINUS_INT) \
    x(UNOP_MINUS_FLOAT) \
    x(UNOP_CMP_NOT_INT) \
    x(UNOP_CMP_NOT_FLOAT) \
    x(UNOP_CMP_NOT_BOOL) \
    x(UNOP_CMP_NOT_REF) \
    x(UNOP_BIT_NOT_INT) \
    x(BRANCH_Z_INT) \
    x(BRANCH_Z_FLOAT) \
    x(BRANCH_Z_BOOL) \
    x(BRANCH_Z_REF) \
    x(BRANCH_NZ_INT) \
    x(BRANCH_NZ_FLOAT) \
    x(BRANCH_NZ_BOOL) \
    x(BRANCH_NZ_REF) \
    x(TEST_INT) \
    x(TEST_FLOAT) \
    x(TEST_REF) \
    x(GET_ATTR_OBJ) \
    x(GET_ATTR_OBJ_REF) \
    x(DUP) \
    x(DUP_REF) \
    x(CALL_VIRTUAL) \

#define OPCODE_DEFINER_TWO(x) \
    x(BINOP_ADD_INT) \
    x(BINOP_ADD_FLOAT) \
    x(BINOP_SUB_INT) \
    x(BINOP_SUB_FLOAT) \
    x(BINOP_MULT_INT) \
    x(BINOP_MULT_FLOAT) \
    x(BINOP_DIV_INT) \
    x(BINOP_DIV_FLOAT) \
    x(BINOP_MOD_INT) \
    x(BINOP_CMP_AND_INT) \
    x(BINOP_CMP_AND_FLOAT) \
    x(BINOP_CMP_AND_BOOL) \
    x(BINOP_CMP_AND_REF) \
    x(BINOP_CMP_OR_INT) \
    x(BINOP_CMP_OR_FLOAT) \
    x(BINOP_CMP_OR_BOOL) \
    x(BINOP_CMP_OR_REF) \
    x(BINOP_CMP_LT_INT) \
    x(BINOP_CMP_LT_FLOAT) \
    x(BINOP_CMP_LT_BOOL) \
    x(BINOP_CMP_LE_INT) \
    x(BINOP_CMP_LE_FLOAT) \
    x(BINOP_CMP_LE_BOOL) \
    x(BINOP_CMP_GT_INT) \
    x(BINOP_CMP_GT_FLOAT) \
    x(BINOP_CMP_GT_BOOL) \
    x(BINOP_CMP_GE_INT) \
    x(BINOP_CMP_GE_FLOAT) \
    x(BINOP_CMP_GE_BOOL) \
    x(BINOP_CMP_EQ_INT) \
    x(BINOP_CMP_EQ_FLOAT) \
    x(BINOP_CMP_EQ_BOOL) \
    x(BINOP_CMP_EQ_REF) \
    x(BINOP_CMP_NEQ_INT) \
    x(BINOP_CMP_NEQ_FLOAT) \
    x(BINOP_CMP_NEQ_BOOL) \
    x(BINOP_CMP_NEQ_REF) \
    x(BINOP_BIT_AND_INT) \
    x(BINOP_BIT_AND_BOOL) \
    x(BINOP_BIT_OR_INT) \
    x(BINOP_BIT_OR_BOOL) \
    x(BINOP_BIT_XOR_INT) \
    x(BINOP_BIT_XOR_BOOL) \
    x(BINOP_BIT_LSHIFT_INT) \
    x(BINOP_BIT_RSHIFT_INT) \
    x(SET_ATTR_OBJ) \
    x(SET_ATTR_OBJ_REF) \
    x(GET_ARRAY_ACCESS) \
    x(GET_ARRAY_ACCESS_REF) \

#define OPCODE_DEFINER_THREE(x) \
    x(SET_ARRAY_ACCESS) \
    x(SET_ARRAY_ACCESS_REF) \

#define OPCODE_DEFINER(x) \
    OPCODE_DEFINER_ZERO(x) \
    OPCODE_DEFINER_ONE(x) \
    OPCODE_DEFINER_TWO(x) \
    OPCODE_DEFINER_THREE(x) \

  enum Opcode {
#define OPND(a) a,
    OPCODE_DEFINER(OPND)
#undef OPND
  };

  static inline void CompileTimeAsserts() {
    enum AnonOpcode {
#define OPND(a) a,
      OPCODE_DEFINER(OPND)
#undef OPND
      NumElems
    };
    // we want the opcode to fit in 1 byte
    VENOM_COMPILE_TIME_ASSERT(NumElems <= 256);
  }

  static std::string stringify(Opcode opcode) {
    switch (opcode) {
#define OPND(a) case a: return #a;
      OPCODE_DEFINER(OPND)
#undef OPND
    default: break;
    }
    VENOM_NOT_REACHED;
  }

  Instruction(Opcode opcode) : opcode(opcode) {}

  /** NOTE: we do *not* need a virtual destructor here,
   * even though we will delete Instruction* pointers which
   * point to subclasses. This is because all subclasses
   * only contain primitive data, so we don't need to call any
   * special destructors (the memory allocator will free the
   * appropriate number of bytes).
   */
  ~Instruction() {}

  /** Execute this instruction given the execution context.  Returns true if
   * the ExecutionContext can simply move on to the next bytecode instruction
   * after execution, or false if this instruction's execution modifies the
   * program_counter */
  bool execute(ExecutionContext& ctx);

private:
  Opcode opcode;

  /** TODO: pass venom_cell by reference or value? Need to benchmark */

#define DECL_ZERO(a) \
  bool a ## _impl(ExecutionContext& ctx);

#define DECL_ONE(a) \
  bool a ## _impl(ExecutionContext& ctx, runtime::venom_cell& opnd0);

#define DECL_TWO(a) \
  bool a ## _impl(ExecutionContext& ctx, runtime::venom_cell& opnd0, \
      runtime::venom_cell& opnd1);

#define DECL_THREE(a) \
  bool a ## _impl(ExecutionContext& ctx, runtime::venom_cell& opnd0, \
      runtime::venom_cell& opnd1, runtime::venom_cell& opnd2);

  OPCODE_DEFINER_ZERO(DECL_ZERO)
  OPCODE_DEFINER_ONE(DECL_ONE)
  OPCODE_DEFINER_TWO(DECL_TWO)
  OPCODE_DEFINER_THREE(DECL_THREE)

#undef DECL_ZERO
#undef DECL_ONE
#undef DECL_TWO
#undef DECL_THREE

 InstFormatU32* asFormatU32();
 InstFormatI32* asFormatI32();
 InstFormatIPtr* asFormatIPtr();
 //InstFormatU32U32* asFormatU32U32();
 InstFormatC* asFormatC();

};

/**
 * WARNING: Subclasses *cannot* contain non-primitive data (anything with
 * a destructor), since we do not have a virtual destructor for
 * Instruction
 *
 * TODO: can we assert this at compile time somehow?
 */

/**
 * An instruction which contains
 *   opcode N0
 *
 * Where N0 is an unsigned int type
 */
class InstFormatU32 : public Instruction {
  friend class Instruction;
public:
  InstFormatU32(Opcode opcode, uint32_t N0) :
    Instruction(opcode), N0(N0) {}
private:
  unsigned int N0;
};

/**
 * An instruction which contains
 *   opcode N0
 *
 * Where N0 is an int type
 */
class InstFormatI32 : public Instruction {
  friend class Instruction;
public:
  InstFormatI32(Opcode opcode, int32_t N0) :
    Instruction(opcode), N0(N0) {}
private:
  int32_t N0;
};

/**
 * An instruction which contains
 *   opcode N0
 *
 * Where N0 is a ptr type
 */
class InstFormatIPtr : public Instruction {
  friend class Instruction;
public:
  InstFormatIPtr(Opcode opcode, intptr_t N0) :
    Instruction(opcode), N0(N0) {}
private:
  intptr_t N0;
};

/**
 * An instruction which contains
 *   opcode N0 N1
 *
 * Where N0 and N1 are unsigned int types
 */
//class InstFormatU32U32 : public Instruction {
//  friend class Instruction;
//public:
//  InstFormatU32U32(Opcode opcode, uint32_t N0, uint32_t N1) :
//    Instruction(opcode), N0(N0), N1(N1) {}
//private:
//  uint32_t N0;
//  uint32_t N1;
//};

/**
 * An instruct which contains
 *   opcode [i64|double|bool]
 */
class InstFormatC : public Instruction {
  friend class Instruction;
public:
  InstFormatC(Opcode opcode, int64_t int_value) :
    Instruction(opcode), data(int_value) {}
  InstFormatC(Opcode opcode, int int_value) :
    Instruction(opcode), data(int_value) {}
  InstFormatC(Opcode opcode, double double_value) :
    Instruction(opcode), data(double_value) {}
  InstFormatC(Opcode opcode, float double_value) :
    Instruction(opcode), data(double_value) {}
  InstFormatC(Opcode opcode, bool bool_value) :
    Instruction(opcode), data(bool_value) {}
private:
  union types {
    /** Primitives */
    int64_t int_value;
    double double_value;
    bool bool_value;

    /** union type constructors */
    types(int64_t int_value) : int_value(int_value) {}
    types(int int_value) : int_value(int_value) {}
    types(double double_value) : double_value(double_value) {}
    types(float double_value) : double_value(double_value) {}
    types(bool bool_value) : bool_value(bool_value) {}
  } data;
};

}
}

#endif /* VENOM_BACKEND_BYTECODE_H */
