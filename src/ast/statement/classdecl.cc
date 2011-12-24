#include <cassert>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <ast/statement/classdecl.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

void ClassDeclNode::registerSymbol(analysis::SemanticContext* ctx) {
  // check to see if this class is already defined in this scope
  ClassSymbol *sym = symbols->findClassSymbol(name, false);
  if (sym) {
    throw SemanticViolationException(
        "Class " + name + " already defined");
  }

  // check to see if all parents are defined
  vector<ClassSymbol*> parSymbols;
  for (TypeStringVec::iterator it = parents.begin();
       it != parents.end(); ++it) {
    ClassSymbol *parsym = symbols->findClassSymbolOrThrow(*it, true);
    parSymbols.push_back(parsym);
  }

  if (parSymbols.empty()) {
    // if no explicit parents declared, parent is object (from the root symbols)
    // TODO: instead of a lookup, we need to store this somewhere
    ClassSymbol *objectType =
      ctx->getRootSymbolTable()->findClassSymbol("object", false);
    assert(objectType);
    parSymbols.push_back(objectType);
  }

  // Define this class's symbol. This MUST happen before semantic checks
  // on the children (to support self-references)
  // TODO: support multiple inheritance
  Type *type = ctx->createType(name, parSymbols.front()->getType(), 0);
  symbols->createClassSymbol(name, type);
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
}

}
}
