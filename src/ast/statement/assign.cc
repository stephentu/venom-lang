#include <cassert>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>

#include <ast/expression/attraccess.h>
#include <ast/expression/functioncall.h>
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
AssignNode::registerSymbol(SemanticContext* ctx) {
  VariableNode *var = dynamic_cast<VariableNode*>(variable);
  if (var) {
    RegisterVariableLHS(ctx, symbols, var);
  }
}

//ASTNode*
//AssignNode::rewriteLocal(SemanticContext* ctx, RewriteMode mode) {
//  switch (mode) {
//  case NonLocalRefs: {
//    if (VariableNode* varLHS = dynamic_cast<VariableNode*>(variable)) {
//      BaseSymbol *bs = varLHS->getSymbol();
//      assert(bs);
//      if (Symbol* sym = dynamic_cast<Symbol*>(bs)) {
//        // TODO: these checks must be the same as the ones in
//        // ast/expression/variable.cc - this is fragile; fix it
//        if (!sym->isLocalTo(symbols) &&
//            !sym->isObjectField() &&
//            !sym->isModuleLevelSymbol()) {
//          // we need to rewrite x to x.set(rhs)
//          // however, we need to process a rewrite on the RHS first
//
//          ASTNode* rep = value->rewriteLocal(ctx, mode);
//          if (rep) {
//            ASTNode *old = value;
//            setNthKid(1, rep);
//            delete old;
//          }
//
//          assert(hasLocationContext(AssignmentLHS));
//          sym->markPromoteToRef();
//
//          FunctionCallNode *rep =
//            new FunctionCallNode(
//              new AttrAccessNode(varLHS->clone(), "set"),
//              TypeStringVec(),
//              util::vec1(value->clone()));
//          return replace(ctx, rep);
//        }
//      }
//    }
//    // fall-through to default case
//  }
//  default: return ASTNode::rewriteLocal(ctx, mode);
//  }
//}

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
  CodeGenAssignment(cg, variable, value);
}

AssignNode*
AssignNode::cloneImpl() {
  return new AssignNode(variable->clone(), value->clone());
}

AssignNode*
AssignNode::cloneForTemplateImpl(const TypeTranslator& t) {
  return new AssignNode(
      variable->cloneForTemplate(t), value->cloneForTemplate(t));
}

void
AssignNode::TypeCheckAssignment(SemanticContext*   ctx,
                                SymbolTable*       symbols,
                                ASTExpressionNode* variable,
                                ASTExpressionNode* value,
                                ClassSymbol*       classSymbol) {
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
    VENOM_ASSERT_TYPEOF_PTR(VariableNode, variable);
    VariableNode *vn = static_cast<VariableNode*>(variable);
    assert(!vn->getExpectedType());
    if (classSymbol) {
      symbols->createClassAttributeSymbol(vn->getName(), rhs, classSymbol);
    } else {
      symbols->createSymbol(vn->getName(), rhs);
    }
    // go again, so we can set the static type on variable
    TypeCheckAssignment(ctx, symbols, variable, value, classSymbol);
  }
}

void
AssignNode::RegisterVariableLHS(SemanticContext* ctx,
                                SymbolTable* symbols,
                                VariableNode* var) {
  // check for duplicate definition (as a function or class)
  if (symbols->isDefined(
        var->getName(), SymbolTable::Function | SymbolTable::Class,
        SymbolTable::NoRecurse)) {
    throw SemanticViolationException(
        "Symbol " + var->getName() + " already defined");
  }

  InstantiatedType* explicitType = var->getExplicitType();
  if (explicitType) {
    // if there is an explicit type string, treat it as
    // explicitly declaring a new symbol
    if (symbols->isDefined(
          var->getName(), SymbolTable::Location, SymbolTable::NoRecurse)) {
      throw SemanticViolationException(
          "Cannot redeclare symbol " + var->getName());
    }
    symbols->createSymbol(var->getName(), explicitType);
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

void
AssignNode::CodeGenAssignment(CodeGenerator& cg,
                              ASTExpressionNode* variable,
                              ASTExpressionNode* value) {
  // TODO: we need to check these conditions actually
  // hold (during static analysis)
  BaseSymbol* bs = variable->getSymbol();
  VENOM_ASSERT_TYPEOF_PTR(Symbol, bs);
  Symbol* sym = static_cast<Symbol*>(bs);

  bool refCnt = variable->getStaticType()->isRefCounted();
  if (sym->isModuleLevelSymbol() || sym->isObjectField()) {
    variable->codeGen(cg);
    value->codeGen(cg);
    size_t slotIdx = sym->getFieldIndex();
    cg.emitInstU32(
        refCnt ?
          Instruction::SET_ATTR_OBJ_REF :
          Instruction::SET_ATTR_OBJ,
        slotIdx);
  } else {
    value->codeGen(cg);
    // no codegen for variable, since we can just store directly
    // into the local variable array
    bool create;
    size_t idx = cg.createLocalVariable(sym, create);
    cg.emitInstU32(
        refCnt ?
          Instruction::STORE_LOCAL_VAR_REF :
          Instruction::STORE_LOCAL_VAR,
        idx);
  }
}

}
}
