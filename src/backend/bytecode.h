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
class InstFormatU32U32;
class InstFormatC;

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
   * onto the stack. The BINOP_ADD handler will be invoked as:
   *
   *   BINOP_ADD_impl(opnd0, opnd1)
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
   *   LOAD_LOCAL_VAR N0
   *      -> variables[N0]
   *   ALLOC_OBJ N0
   *      -> obj
   *   CALL N0
   *      -> ret_pc ; pc = N0
   *   JUMP N0
   *      -> ; pc = next_pc + N0
   *
   * One operand instructions:
   *
   *   POP_CELL
   *     opnd0 ->
   *   STORE_LOCAL_VAR N0
   *     opnd0 -> ; variables[N0] = opnd0
   *   UNOP_PLUS
   *     opnd0 -> +opnd0
   *   UNOP_PLUS_FLOAT
   *     opnd0 -> +opnd0
   *   UNOP_MINUS
   *     opnd0 -> -opnd0
   *   UNOP_MINUS_FLOAT
   *     opnd0 -> -opnd0
   *   UNOP_CMP_NOT
   *     opnd0 -> !opnd0
   *   UNOP_BIT_NOT
   *     opnd0 -> ~opnd0
   *   BRANCH_Z N0
   *     opnd0 -> ; if (opnd) pc = next_pc + N0 else pc = next_pc
   *   BRANCH_NZ N0
   *     opnd0 -> ; if (!opnd) pc = next_pc + N0 else pc = next_pc
   *   GET_ATTR_OBJ N0
   *     opnd0 -> opnd0.attr[N0]
   *   GET_ARRAY_ACCESS
   *   DUP N0
   *     opnd0 -> opnd0, opnd0, ..., opnd0 (N0 + 1 instances)
   *
   * Two operand instructions:
   *
   *   BINOP_ADD
   *     opnd0, opnd1 -> opnd0 + opnd1
   *   BINOP_ADD_FLOAT
   *     opnd0, opnd1 -> opnd0 + opnd1
   *   BINOP_SUB
   *     opnd0, opnd1 -> opnd0 - opnd1
   *   BINOP_SUB_FLOAT
   *     opnd0, opnd1 -> opnd0 - opnd1
   *   BINOP_MULT
   *     opnd0, opnd1 -> opnd0 * opnd1
   *   BINOP_MULT_FLOAT
   *     opnd0, opnd1 -> opnd0 * opnd1
   *   BINOP_DIV
   *     opnd0, opnd1 -> opnd0 / opnd1
   *   BINOP_DIV_FLOAT
   *     opnd0, opnd1 -> opnd0 / opnd1
   *   BINOP_CMP_AND
   *     opnd0, opnd1 -> opnd0 && opnd1
   *   BINOP_CMP_OR
   *     opnd0, opnd1 -> opnd0 || opnd1
   *   BINOP_CMP_LT
   *     opnd0, opnd1 -> opnd0 < opnd1
   *   BINOP_CMP_LE
   *     opnd0, opnd1 -> opnd0 <= opnd1
   *   BINOP_CMP_GT
   *     opnd0, opnd1 -> opnd0 > opnd1
   *   BINOP_CMP_GE
   *     opnd0, opnd1 -> opnd0 >= opnd1
   *   BINOP_CMP_EQ
   *     opnd0, opnd1 -> opnd0 == opnd1
   *   BINOP_CMP_NEQ
   *     opnd0, opnd1 -> opnd0 != opnd1
   *   BINOP_BIT_AND
   *     opnd0, opnd1 -> opnd0 & opnd1
   *   BINOP_BIT_OR
   *     opnd0, opnd1 -> opnd0 | opnd1
   *   BINOP_BIT_XOR
   *     opnd0, opnd1 -> opnd0 ^ opnd1
   *   BINOP_BIT_LSHIFT
   *     opnd0, opnd1 -> opnd0 << opnd1
   *   BINOP_BIT_RSHIFT
   *     opnd0, opnd1 -> opnd0 >> opnd1
   *   SET_ATTR_OBJ N0
   *     opnd0, opnd1 -> ; opnd0.attr[N0] = opnd1
   *   SET_ARRAY_ACCESS
   *   RET
   *     opnd0, opnd1 -> opnd1 ; PC = opnd0
   */

#define OPCODE_DEFINER_ZERO(x) \
    x(PUSH_CELL_INT) \
    x(PUSH_CELL_FLOAT) \
    x(PUSH_CELL_BOOL) \
    x(PUSH_CELL_NIL) \
    x(LOAD_LOCAL_VAR) \
    x(ALLOC_OBJ) \
    x(CALL) \
    x(JUMP) \

#define OPCODE_DEFINER_ONE(x) \
    x(POP_CELL) \
    x(STORE_LOCAL_VAR) \
    x(UNOP_PLUS) \
    x(UNOP_PLUS_FLOAT) \
    x(UNOP_MINUS) \
    x(UNOP_MINUS_FLOAT) \
    x(UNOP_CMP_NOT) \
    x(UNOP_BIT_NOT) \
    x(BRANCH_Z) \
    x(BRANCH_NZ) \
    x(GET_ATTR_OBJ) \
    x(GET_ARRAY_ACCESS) \
    x(DUP) \

#define OPCODE_DEFINER_TWO(x) \
    x(BINOP_ADD) \
    x(BINOP_ADD_FLOAT) \
    x(BINOP_SUB) \
    x(BINOP_SUB_FLOAT) \
    x(BINOP_MULT) \
    x(BINOP_MULT_FLOAT) \
    x(BINOP_DIV) \
    x(BINOP_DIV_FLOAT) \
    x(BINOP_CMP_AND) \
    x(BINOP_CMP_OR) \
    x(BINOP_CMP_LT) \
    x(BINOP_CMP_LE) \
    x(BINOP_CMP_GT) \
    x(BINOP_CMP_GE) \
    x(BINOP_CMP_EQ) \
    x(BINOP_CMP_NEQ) \
    x(BINOP_BIT_AND) \
    x(BINOP_BIT_OR) \
    x(BINOP_BIT_XOR) \
    x(BINOP_BIT_LSHIFT) \
    x(BINOP_BIT_RSHIFT) \
    x(SET_ATTR_OBJ) \
    x(SET_ARRAY_ACCESS) \
    x(RET) \

#define OPCODE_DEFINER(x) \
    OPCODE_DEFINER_ZERO(x) \
    OPCODE_DEFINER_ONE(x) \
    OPCODE_DEFINER_TWO(x) \

  enum Opcode {
#define OPND(a) a,
    OPCODE_DEFINER(OPND)
#undef OPND
  };

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

  /** Execute this instruction given the execution context.  Returns true if
   * the ExecutionContext can simply move on to the next bytecode instruction
   * after execution, or false if this instruction's execution modifies the
   * program_counter */
  bool execute(ExecutionContext& ctx);

private:
  Opcode opcode;

#define DECL_ZERO(a) \
  bool a ## _impl(ExecutionContext& ctx);

#define DECL_ONE(a) \
  bool a ## _impl(ExecutionContext& ctx, runtime::venom_cell& opnd0);

#define DECL_TWO(a) \
  bool a ## _impl(ExecutionContext& ctx, runtime::venom_cell& opnd0, \
      runtime::venom_cell& opnd1);

  OPCODE_DEFINER_ZERO(DECL_ZERO)
  OPCODE_DEFINER_ONE(DECL_ONE)
  OPCODE_DEFINER_TWO(DECL_TWO)

#undef DECL_ZERO
#undef DECL_ONE
#undef DECL_TWO

 InstFormatU32* asFormatU32();
 InstFormatI32* asFormatI32();
 InstFormatU32U32* asFormatU32U32();
 InstFormatC* asFormatC();

};

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
 *   opcode N0 N1
 *
 * Where N0 and N1 are unsigned int types
 */
class InstFormatU32U32 : public Instruction {
  friend class Instruction;
public:
  InstFormatU32U32(Opcode opcode, uint32_t N0, uint32_t N1) :
    Instruction(opcode), N0(N0), N1(N1) {}
private:
  uint32_t N0;
  uint32_t N1;
};

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
