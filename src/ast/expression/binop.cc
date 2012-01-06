#include <ast/expression/binop.h>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>

#include <backend/codegenerator.h>

using namespace std;
using namespace venom::analysis;
using namespace venom::backend;

namespace venom {
namespace ast {

string BinopNode::StringifyType(Type type) {
  switch (type) {
  case ADD: return "+";
  case SUB: return "-";
  case MULT: return "*";
  case DIV: return "/";
  case MOD: return "%";
  case CMP_AND: return "and";
  case CMP_OR: return "or";
  case CMP_LT: return "<";
  case CMP_LE: return "<=";
  case CMP_GT: return ">";
  case CMP_GE: return ">=";
  case CMP_EQ: return "==";
  case CMP_NEQ: return "!=";
  case BIT_AND: return "&";
  case BIT_OR: return "|";
  case BIT_XOR: return "^";
  case BIT_LSHIFT: return "<<";
  case BIT_RSHIFT: return ">>";
  default: assert(false);
  }
  return "";
}

bool BinopNode::isShortCircuitOp() const {
  switch (type) {
  case CMP_AND: case CMP_OR: return true;
  default: return false;
  }
}

InstantiatedType*
BinopNode::typeCheckImpl(SemanticContext* ctx,
                         InstantiatedType* expected,
                         const InstantiatedTypeVec& typeParamArgs) {
  InstantiatedType *lhs = left->typeCheck(ctx);
  InstantiatedType *rhs = right->typeCheck(ctx);

  switch (type) {
  case ADD:
    // special case for str concat
    if (lhs->isString() || rhs->isString()) {
      if (!lhs->equals(*rhs)) {
        throw TypeViolationException(
            "Cannot concat type " + lhs->stringify() +
            " with type " + rhs->stringify());
      }
      return lhs;
    }

  case SUB:
  case MULT:
  case DIV:
    // allow (double op int) or (int op double), with silent promotion
    if (!lhs->isNumeric()) {
      throw TypeViolationException(
          "Expected int or double type with operator " +
          StringifyType(type) + ", got " + lhs->stringify());
    }
    if (!rhs->isNumeric()) {
      throw TypeViolationException(
          "Expected int or double type with operator " +
          StringifyType(type) + ", got " + rhs->stringify());
    }
    if (lhs->isFloat()) return lhs;
    if (rhs->isFloat()) return rhs;
    return lhs;

  case MOD:
  case BIT_LSHIFT:
  case BIT_RSHIFT:
    // require both ops to be ints
    if (!lhs->isInt() || !rhs->isInt()) {
      throw TypeViolationException(
          "Expected int arguments to operator " + StringifyType(type) +
          ", got " + lhs->stringify() + " and " + rhs->stringify());
    }
    return InstantiatedType::IntType;

  case BIT_AND:
  case BIT_OR:
  case BIT_XOR:
    // require both ops to be equal and either both ints or bools
    if (!lhs->isInt() && !lhs->isBool()) {
      throw TypeViolationException(
          "Expected int or bool for operator " +
          StringifyType(type) + ", got " + lhs->stringify());
    }
    if (!rhs->isInt() && !rhs->isBool()) {
      throw TypeViolationException(
          "Expected int or bool for operator " +
          StringifyType(type) + ", got " + rhs->stringify());
    }
    if (!lhs->equals(*rhs)) {
      throw TypeViolationException(
          "Expecting equal types to operator " + StringifyType(type) + ", got " +
          lhs->stringify() + " and " + rhs->stringify());
    }
    return lhs;

  case CMP_AND:
  case CMP_OR:
    // any type can be here
    return InstantiatedType::BoolType;

  case CMP_EQ:
  case CMP_NEQ:
    // TODO: catch impossible (in)equalities
    return InstantiatedType::BoolType;

  case CMP_LT:
  case CMP_LE:
  case CMP_GT:
  case CMP_GE:
    // require types to be either primitive or string, and equal
    if (!lhs->isPrimitive() && !lhs->isString()) {
      throw TypeViolationException(
          "Expected primitive or string type for operator " +
          StringifyType(type) + ", got " + lhs->stringify());
    }
    if (!rhs->isPrimitive() && !rhs->isString()) {
      throw TypeViolationException(
          "Expected primitive or string type for operator " +
          StringifyType(type) + ", got " + rhs->stringify());
    }

    if (!lhs->equals(*rhs)) {
      throw TypeViolationException(
          "Expecting equal types to operator " + StringifyType(type) + ", got " +
          lhs->stringify() + " and " + rhs->stringify());
    }
    // TODO: rewrite string case to call string method
    return InstantiatedType::BoolType;
  default:
    assert(false);
  }
  return NULL;
}

void
BinopNode::codeGen(CodeGenerator& cg) {
  left->codeGen(cg);
  switch (type) {
  case ADD: {
    if (left->getStaticType()->isString()) {
      // TODO: str concat
      VENOM_UNIMPLEMENTED;
    }
  }
  case SUB: case MULT: case DIV: {
    bool leftIsFloat = left->getStaticType()->isFloat();
    bool rightIsFloat = right->getStaticType()->isFloat();
    bool promote = leftIsFloat || rightIsFloat;
    if (promote && !leftIsFloat) {
      cg.emitInst(Instruction::INT_TO_FLOAT);
    }
    right->codeGen(cg);
    if (promote && !rightIsFloat) {
      cg.emitInst(Instruction::INT_TO_FLOAT);
    }
    switch (type) {
    case ADD:
      cg.emitInst(!promote ? Instruction::BINOP_ADD_INT :
                             Instruction::BINOP_ADD_FLOAT);
      break;
    case SUB:
      cg.emitInst(!promote ? Instruction::BINOP_SUB_INT :
                             Instruction::BINOP_SUB_FLOAT);
      break;
    case MULT:
      cg.emitInst(!promote ? Instruction::BINOP_MULT_INT :
                             Instruction::BINOP_MULT_FLOAT);
      break;
    case DIV:
      cg.emitInst(!promote ? Instruction::BINOP_DIV_INT :
                             Instruction::BINOP_DIV_FLOAT);
      break;
    default: assert(false);
    }
    break;
  }

  case MOD:
  case BIT_LSHIFT:
  case BIT_RSHIFT: {
    assert(left->getStaticType()->isInt());
    assert(right->getStaticType()->isInt());
    right->codeGen(cg);
    Instruction::Opcode op;
    switch (type) {
    case MOD: op = Instruction::BINOP_MOD_INT; break;
    case BIT_LSHIFT: op = Instruction::BINOP_BIT_LSHIFT_INT; break;
    case BIT_RSHIFT: op = Instruction::BINOP_BIT_RSHIFT_INT; break;
    default: assert(false);
    }
    cg.emitInst(op);
    break;
  }

  case CMP_LT:
  case CMP_LE:
  case CMP_GT:
  case CMP_GE: {
    // string case handled separately
    assert(left->getStaticType()->isPrimitive());
    assert(right->getStaticType()->isPrimitive());
    assert(left->getStaticType()->equals(*right->getStaticType()));
    right->codeGen(cg);

    Instruction::Opcode op;
#define DO_OPCODE_SWITCH(expr_type) \
  do { \
    switch (type) { \
    case CMP_LT: op = Instruction::BINOP_CMP_LT_##expr_type; break; \
    case CMP_LE: op = Instruction::BINOP_CMP_LE_##expr_type; break; \
    case CMP_GT: op = Instruction::BINOP_CMP_GT_##expr_type; break; \
    case CMP_GE: op = Instruction::BINOP_CMP_GE_##expr_type; break; \
    default: assert(false); \
    } \
  } while (0)

    if (left->getStaticType()->isInt()) {
      DO_OPCODE_SWITCH(INT);
    } else if (left->getStaticType()->isFloat()) {
      DO_OPCODE_SWITCH(FLOAT);
    } else if (left->getStaticType()->isBool()) {
      DO_OPCODE_SWITCH(BOOL);
    } else assert(false);

    cg.emitInst(op);

#undef DO_OPCODE_SWITCH

    break;
  }

  case CMP_EQ:
  case CMP_NEQ: {
    right->codeGen(cg);
    if (left->getStaticType()->equals(*right->getStaticType())) {
#define DO_EMIT(expr_type) \
  cg.emitInst(type == CMP_EQ ? Instruction::BINOP_CMP_EQ_##expr_type : \
                               Instruction::BINOP_CMP_NEQ_##expr_type)

      if (left->getStaticType()->isInt()) {
        DO_EMIT(INT);
      } else if (left->getStaticType()->isFloat()) {
        DO_EMIT(FLOAT);
      } else if (left->getStaticType()->isBool()) {
        DO_EMIT(BOOL);
      } else {
        DO_EMIT(REF);
      }

#undef DO_EMIT
    } else {
      // TODO: fix this implementation limitation for now...
      // this will require us to box primitives (if one of the types
      // is ref counted). also the other case (if we do something like
      // True == 1.0) requires primitive type conversion.
      assert(left->getStaticType()->isRefCounted());
      assert(right->getStaticType()->isRefCounted());
      cg.emitInst(type == CMP_EQ ? Instruction::BINOP_CMP_EQ_REF :
                                   Instruction::BINOP_CMP_NEQ_REF);
    }
    break;
  }

  case BIT_AND:
  case BIT_OR:
  case BIT_XOR: {
    assert(left->getStaticType()->isInt() ||
           left->getStaticType()->isBool());
    assert(right->getStaticType()->isInt() ||
           right->getStaticType()->isBool());
    assert(left->getStaticType()->equals(*right->getStaticType()));

    right->codeGen(cg);

    Instruction::Opcode op;
#define DO_OPCODE_SWITCH(expr_type) \
  do { \
    switch (type) { \
    case BIT_AND: op = Instruction::BINOP_BIT_AND_##expr_type; break; \
    case BIT_OR: op = Instruction::BINOP_BIT_OR_##expr_type; break; \
    case BIT_XOR: op = Instruction::BINOP_BIT_XOR_##expr_type; break; \
    default: assert(false); \
    } \
  } while (0)

    if (left->getStaticType()->isInt()) {
      DO_OPCODE_SWITCH(INT);
    } else if (left->getStaticType()->isBool()) {
      DO_OPCODE_SWITCH(BOOL);
    } else assert(false);

    cg.emitInst(op);

#undef DO_OPCODE_SWITCH
    break;
  }

  case CMP_AND:
  case CMP_OR: {
    Label *shortCircuit = cg.newLabel();
    Label *done = cg.newLabel();

    Instruction::Opcode opZ, opNZ;
    if (left->getStaticType()->isInt()) {
      opZ = Instruction::BRANCH_Z_INT;
      opNZ = Instruction::BRANCH_NZ_INT;
    } else if (left->getStaticType()->isFloat()) {
      opZ = Instruction::BRANCH_Z_FLOAT;
      opNZ = Instruction::BRANCH_NZ_FLOAT;
    } else if (left->getStaticType()->isBool()) {
      opZ = Instruction::BRANCH_Z_BOOL;
      opNZ = Instruction::BRANCH_NZ_BOOL;
    } else {
      opZ = Instruction::BRANCH_Z_REF;
      opNZ = Instruction::BRANCH_NZ_REF;
    }

    cg.emitInstLabel(type == CMP_AND ? opZ : opNZ, shortCircuit);
    right->codeGen(cg);
    if (!right->getStaticType()->isBool()) {
      Instruction::Opcode opT;
      if (left->getStaticType()->isInt()) {
        opT = Instruction::TEST_INT;
      } else if (left->getStaticType()->isFloat()) {
        opT = Instruction::TEST_FLOAT;
      } else {
        opT = Instruction::TEST_REF;
      }
      cg.emitInst(opT);
    }

    cg.emitInstLabel(Instruction::JUMP, done);
    cg.bindLabel(shortCircuit);
    cg.emitInstBool(Instruction::PUSH_CELL_BOOL, type == CMP_OR);
    cg.bindLabel(done);
    break;
  }

  default: assert(false);
  }
}

}
}
