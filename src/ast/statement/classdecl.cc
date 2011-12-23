#include <cassert>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <ast/statement/classdecl.h>

using namespace std;
using namespace venom::analysis;

namespace venom {
namespace ast {

void ClassDeclNode::semanticCheck(SemanticContext* ctx) {
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
    const ParameterizedTypeString* failed_type;
    bool wrong_params;
    ClassSymbol *parsym =
      symbols->findClassSymbol(*it, true, failed_type, wrong_params);
    if (!parsym) {
      assert(failed_type);
      if (wrong_params) {
        throw SemanticViolationException(
            "Wrong number of type parameters given to " + failed_type->name);
      } else {
        throw SemanticViolationException(
            "Type " + failed_type->name + " not defined");
      }
    } else {
      parSymbols.push_back(parsym);
    }
  }

  if (parSymbols.empty()) {
    // if no explicit parents declared, parent is object
    // TODO: instead of a lookup, we need to store this somewhere
    ClassSymbol *objectType = symbols->findClassSymbol("object", true);
    assert(objectType);
    parSymbols.push_back(objectType);
  }

  // Define this class's symbol. This MUST happen before semantic checks
  // on the children (to support self-references)
  // TODO: support multiple inheritance
  Type *type = ctx->createType(name, parSymbols.front()->getType(), 0);
  symbols->createClassSymbol(name, type);

  ASTNode::semanticCheck(ctx);
}

}
}
