#include <ast/expression/unop.h>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

string UnopNode::StringifyType(Type type) {
  switch (type) {
  case PLUS: return "+";
  case MINUS: return "-";
  case CMP_NOT: return "not";
  case BIT_NOT: return "~";
  default: assert(false);
  }
  return "";
}

InstantiatedType*
UnopNode::typeCheck(SemanticContext* ctx,
                    InstantiatedType* expected,
                    const InstantiatedTypeVec& typeParamArgs) {
  InstantiatedType *kidType = kid->typeCheck(ctx);

  switch (type) {
  case PLUS:
  case MINUS:
    // require numeric or bool
    if (!kidType->isNumeric() &&
        !kidType->equals(*InstantiatedType::BoolType)) {
      throw TypeViolationException(
          "Expected numeric type or bool for unary operator " +
          StringifyType(type) + ", got " + kidType->stringify());
    }
    return kidType;

  case BIT_NOT:
    // require int
    if (!kidType->equals(*InstantiatedType::IntType)) {
      throw TypeViolationException(
          "Expected type int for unary operator " + StringifyType(type) +
          ", got " + kidType->stringify());
    }
    return kidType;

  case CMP_NOT:
    // allow any type
    return InstantiatedType::BoolType;

  default: assert(false);
  }
  return NULL;
}

}
}