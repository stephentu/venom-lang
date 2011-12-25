#include <stdexcept>

#include <ast/statement/classdecl.h>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>

using namespace std;
using namespace venom::ast;

namespace venom {
namespace analysis {

bool FuncSymbol::isConstructor() const {
  const ClassDeclNode *cdn =
    dynamic_cast<const ClassDeclNode*>(getDefinedSymbolTable()->getOwner());
  if (!cdn) return false;
  return (cdn->getName() == name);
}

bool FuncSymbol::isMethod() const {
  const ClassDeclNode *cdn =
    dynamic_cast<const ClassDeclNode*>(getDefinedSymbolTable()->getOwner());
  return cdn;
}

InstantiatedType*
FuncSymbol::bind(SemanticContext* ctx,
                 const vector<InstantiatedType*>& params) {
  // TODO: actually bind params

  Type *ftypes[] = {
    Type::Func0Type,
    Type::Func1Type,
    Type::Func2Type,
    Type::Func3Type,
    Type::Func4Type,
    Type::Func5Type,
    Type::Func6Type,
    Type::Func7Type,
    Type::Func8Type,
    Type::Func9Type,
    Type::Func10Type,
    Type::Func11Type,
    Type::Func12Type,
    Type::Func13Type,
    Type::Func14Type,
    Type::Func15Type,
    Type::Func16Type,
    Type::Func17Type,
    Type::Func18Type,
    Type::Func19Type,
  };

  if (this->params.size() >= VENOM_NELEMS(ftypes)) {
    // TODO: better error message
    throw runtime_error("Too many parameters");
  }

  vector<InstantiatedType*> fparams(this->params);
  fparams.push_back(returnType);
  return ftypes[params.size()]->instantiate(ctx, fparams);
}

InstantiatedType*
ClassSymbol::bind(SemanticContext* ctx,
                  const vector<InstantiatedType*>& params) {
  VENOM_UNIMPLEMENTED;
}

}
}
