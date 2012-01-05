#include <cassert>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>

#include <ast/expression/variable.h>
#include <ast/statement/assign.h>

#include <backend/bytecode.h>
#include <backend/codegenerator.h>

using namespace std;
using namespace venom::analysis;
using namespace venom::backend;

namespace venom {
namespace ast {

void
AssignNode::TypeCheckAssignment(SemanticContext*   ctx,
                                SymbolTable*       symbols,
                                ASTExpressionNode* variable,
                                ASTExpressionNode* value) {
  InstantiatedType *lhs = variable->typeCheck(ctx, NULL);
  InstantiatedType *rhs = value->typeCheck(ctx, lhs);
  assert(rhs);
  if (!rhs->getType()->isVisible()) {
    throw TypeViolationException(
        "Cannot create reference to hidden type " + rhs->stringify());
  }

  if (lhs) {
    // require rhs <: lhs
    if (!rhs->isSubtypeOf(*lhs)) {
      throw TypeViolationException(
          "Cannot assign type " + rhs->stringify() + " to type " + lhs->stringify());
    }
  } else {
    VariableNode *vn = dynamic_cast<VariableNode*>(variable);
    assert(vn && !vn->getExplicitParameterizedTypeString());
    symbols->createSymbol(vn->getName(), rhs);
  }
}

void
AssignNode::registerSymbol(SemanticContext* ctx) {
  VariableNode *var = dynamic_cast<VariableNode*>(variable);
  if (var) {
    // check for duplicate definition (as a function or class)
    if (symbols->isDefined(
          var->getName(), SymbolTable::Function | SymbolTable::Class,
          SymbolTable::NoRecurse)) {
      throw SemanticViolationException(
          "Symbol " + var->getName() + " already defined");
    }

    if (var->getExplicitParameterizedTypeString()) {
      // if there is an explicit type string, treat it as
      // explicitly declaring a new symbol
      if (symbols->isDefined(
            var->getName(), SymbolTable::Location, SymbolTable::NoRecurse)) {
        throw SemanticViolationException(
            "Cannot redeclare symbol " + var->getName());
      }
      InstantiatedType *itype = ctx->instantiateOrThrow(
            symbols, var->getExplicitParameterizedTypeString());
      symbols->createSymbol(var->getName(), itype);
    } else {
      // if there is no type string, then only create a new
      // declaration if the symbol doesn't exist anywhere in the scope
      // (current or parents) as a location type
      //
      // for example, consider case 1:
      // class A
      //   attr x::int
      // end
      // class B <- A
      //   def self(y::int) =
      //     x = y; # does *NOT* declare a new symbol
      //   end
      // end
      //
      // versus:
      //
      // class A
      //   def x() = print('hi'); end
      // end
      // class B <- A
      //   def self(y::int) =
      //     x = y; # *does* declare a new symbol
      //   end
      // end
      if (!symbols->isDefined(
            var->getName(), SymbolTable::Location,
            SymbolTable::AllowCurrentScope)) {
        symbols->createSymbol(var->getName(), NULL);
      }
    }
  }
}

void
AssignNode::semanticCheckImpl(SemanticContext* ctx, bool doRegister) {
  // Do the right child first (this prevents recursive assignment, ie x = x)
  if (value) value->semanticCheckImpl(ctx, true);
  if (doRegister) {
    registerSymbol(ctx);
  }
  // dont recurse on variable...
}

void
AssignNode::typeCheck(SemanticContext* ctx, InstantiatedType* expected) {
  assert(value);
  TypeCheckAssignment(ctx, symbols, variable, value);
  checkExpectedType(expected);
}

void
AssignNode::codeGen(CodeGenerator& cg) {
  value->codeGen(cg);

  // TODO: we need to check these conditions actually
  // hold (during static analysis)
  BaseSymbol* bs = variable->getSymbol();
  assert(bs);
  Symbol* sym = dynamic_cast<Symbol*>(bs);
  assert(sym);

  size_t idx = cg.createLocalVariable(sym);
  cg.emitInstU32(Instruction::STORE_LOCAL_VAR, idx);
}

}
}
