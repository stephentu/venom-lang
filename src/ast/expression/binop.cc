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
    if (lhs->equals(*InstantiatedType::StringType) ||
        rhs->equals(*InstantiatedType::StringType)) {
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
    if (lhs->equals(*InstantiatedType::FloatType)) return lhs;
    if (rhs->equals(*InstantiatedType::FloatType)) return rhs;
    return lhs;

  case MOD:
  case BIT_AND:
  case BIT_OR:
  case BIT_XOR:
  case BIT_LSHIFT:
  case BIT_RSHIFT:
    // require both ops to be ints
    if (!lhs->equals(*InstantiatedType::IntType) ||
        !rhs->equals(*InstantiatedType::IntType)) {
      throw TypeViolationException(
          "Expected int arguments to operator " + StringifyType(type) +
          ", got " + lhs->stringify() + " and " + rhs->stringify());
    }
    return InstantiatedType::IntType;

  case CMP_AND:
  case CMP_OR:
  case CMP_EQ:
  case CMP_NEQ:
    // any type can be here
    return InstantiatedType::BoolType;

  case CMP_LT:
  case CMP_LE:
  case CMP_GT:
  case CMP_GE:
    // require types to be either numeric, bool, or string, and equal
    if (!lhs->isNumeric() &&
        !lhs->equals(*InstantiatedType::BoolType) &&
        !lhs->equals(*InstantiatedType::StringType)) {
      throw TypeViolationException(
          "Expected numeric, bool, or string type for operator " +
          StringifyType(type) + ", got " + lhs->stringify());
    }
    if (!rhs->isNumeric() &&
        !rhs->equals(*InstantiatedType::BoolType) &&
        !rhs->equals(*InstantiatedType::StringType)) {
      throw TypeViolationException(
          "Expected numeric, bool, or string type for operator " +
          StringifyType(type) + ", got " + rhs->stringify());
    }

    if (!lhs->equals(*rhs)) {
      throw TypeViolationException(
          "Expecting equal types to operator " + StringifyType(type) + ", got " +
          lhs->stringify() + " and " + rhs->stringify());
    }

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
    if (left->getStaticType()->getType()->isString()) {
      // TODO: str concat
      VENOM_UNIMPLEMENTED;
    }
  }
  case SUB: case MULT: case DIV: {
    bool leftIsFloat = left->getStaticType()->getType()->isFloat();
    bool rightIsFloat = right->getStaticType()->getType()->isFloat();
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
      cg.emitInst(!promote ? Instruction::BINOP_ADD :
                             Instruction::BINOP_ADD_FLOAT);
      break;
    case SUB:
      cg.emitInst(!promote ? Instruction::BINOP_SUB :
                             Instruction::BINOP_SUB_FLOAT);
      break;
    case MULT:
      cg.emitInst(!promote ? Instruction::BINOP_MULT :
                             Instruction::BINOP_MULT_FLOAT);
      break;
    case DIV:
      cg.emitInst(!promote ? Instruction::BINOP_DIV :
                             Instruction::BINOP_DIV_FLOAT);
      break;
    default: assert(false);
    }
    break;
  }
  case MOD:
  case CMP_LT:
  case CMP_LE:
  case CMP_GT:
  case CMP_GE:
  case CMP_EQ:
  case CMP_NEQ:
  case BIT_AND:
  case BIT_OR:
  case BIT_XOR:
  case BIT_LSHIFT:
  case BIT_RSHIFT: {
    right->codeGen(cg);
    Instruction::Opcode op;
#define OP_CASE(m) case m: op = Instruction::BINOP_ ## m; break;
    switch (type) {
      OP_CASE(MOD)
      OP_CASE(CMP_LT)
      OP_CASE(CMP_LE)
      OP_CASE(CMP_GT)
      OP_CASE(CMP_GE)
      OP_CASE(CMP_EQ)
      OP_CASE(CMP_NEQ)
      OP_CASE(BIT_AND)
      OP_CASE(BIT_OR)
      OP_CASE(BIT_XOR)
      OP_CASE(BIT_LSHIFT)
      OP_CASE(BIT_RSHIFT)
      default: assert(false);
    }
#undef OP_CASE
    cg.emitInst(op);
    break;
  }
  case CMP_AND:
  case CMP_OR: {
    Label *shortCircuit = cg.newLabel();
    Label *done = cg.newLabel();
    cg.emitInstLabel(
        type == CMP_AND ? Instruction::BRANCH_Z : Instruction::BRANCH_NZ,
        shortCircuit);
    right->codeGen(cg);
    cg.emitInst(Instruction::TEST);
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
