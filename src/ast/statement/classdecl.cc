#include <cassert>
#include <algorithm>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>

#include <ast/statement/classdecl.h>
#include <ast/statement/funcdecl.h>
#include <ast/statement/stmtlist.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

struct functor {
  functor(SemanticContext* ctx, SymbolTable* st)
    : ctx(ctx), st(st) {}
  inline
  InstantiatedType* operator()(const ParameterizedTypeString* t) const {
    return ctx->instantiateOrThrow(st, t);
  }
  SemanticContext* ctx;
  SymbolTable*     st;
};

void ClassDeclNode::registerSymbol(SemanticContext* ctx) {
  // check to see if this class is already defined in this scope
  if (symbols->isDefined(name, SymbolTable::Any, false)) {
    throw SemanticViolationException(
        "Class " + name + " already defined");
  }

  if (parents.size() > 1) {
    throw SemanticViolationException(
        "Multiple inheritance currently not supported");
  }

  // check to see if all parents are defined
  vector<InstantiatedType*> parentTypes(parents.size());
  transform(parents.begin(), parents.end(),
            parentTypes.begin(), functor(ctx, symbols));

  if (parents.empty()) {
    // if no explicit parents declared, parent is object
    parentTypes.push_back(InstantiatedType::ObjectType);
  }

  // Define this class's symbol. This MUST happen before semantic checks
  // on the children (to support self-references)
  // TODO: support multiple inheritance
  // TODO: support type parameters
  Type *type = ctx->createType(name, parentTypes.front(), 0);
  symbols->createClassSymbol(name, stmts->getSymbolTable(), type);

  // link the stmts symbol table to the parents symbol tables
  for (vector<InstantiatedType*>::iterator it = parentTypes.begin();
       it != parentTypes.end(); ++it) {
    stmts->getSymbolTable()->addParent((*it)->getClassSymbolTable());
  }
}

void ClassDeclNode::semanticCheckImpl(SemanticContext* ctx, bool doRegister) {
  if (doRegister) {
    registerSymbol(ctx);
  }

  // register all the children first
  forchild (kid) {
    kid->registerSymbol(ctx);
  } endfor

  // now recurse on children w/o registration
  forchild (kid) {
    kid->semanticCheckImpl(ctx, false);
  } endfor

  // now look for a ctor definition
  if (!stmts->getSymbolTable()->findFuncSymbol(name, false)) {
    // no ctor defined, insert a default one
    ASTStatementNode *ctor =
      new FuncDeclNode(
          name,
          ExprNodeVec(),
          new ParameterizedTypeString("void"),
          new StmtListNode);

    ctor->initSymbolTable(stmts->getSymbolTable());
    ctor->registerSymbol(ctx);
    ctor->semanticCheckImpl(ctx, false); // technically not needed,
                                         // since empty body

    dynamic_cast<StmtListNode*>(stmts)->appendStatement(ctor);
  }
  assert(stmts->getSymbolTable()->findFuncSymbol(name, false));
}

}
}
