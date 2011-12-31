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

class Instruction {
public:

#define OPCODE_DEFINER_ZERO(x) \
		x(POP_CELL) \
		x(PUSH_CELL) \
		x(ALLOC_OBJ) \
		x(CALL) \

#define OPCODE_DEFINER_ONE(x) \
		x(UNOP_PLUS) \
		x(UNOP_PLUS_FLOAT) \
		x(UNOP_MINUS) \
		x(UNOP_MINUS_FLOAT) \
		x(UNOP_CMP_NOT) \
		x(UNOP_BIT_NOT) \
		x(BRANCH_Z) \
		x(BRANCH_NZ) \
		x(ATTR_OBJ) \
		x(RET) \

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

};

/**
 * An instruction which contains
 *   opcode N0
 *
 * Where N0 is an unsigned int type
 */
class InstFormatA : public Instruction {
	friend class Instruction;
public:
	InstFormatA(Opcode opcode, size_t N0) :
		Instruction(opcode), N0(N0) {}
private:
	size_t N0;
};

/**
 * An instruction which contains
 *   opcode N0 N1
 *
 * Where N0 and N1 are unsigned int types
 */
class InstFormatB : public Instruction {
	friend class Instruction;
public:
	InstFormatB(Opcode opcode, size_t N0, size_t N1) :
		Instruction(opcode), N0(N0), N1(N1) {}
private:
	size_t N0;
	size_t N1;
};

/**
 * An instruct which contains
 *   opcode primitive_cell
 */
class InstFormatC : public Instruction {
	friend class Instruction;
public:
	InstFormatC(Opcode opcode, const runtime::venom_cell& cell) :
		Instruction(opcode), cell(cell) {
		assert(cell.isPrimitive());
	}
private:
	runtime::venom_cell cell;
};

}
}

#endif /* VENOM_BACKEND_BYTECODE_H */
