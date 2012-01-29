#include <cassert>
#include <sstream>

#include <analysis/boundfunction.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>

#include <util/stl.h>

using namespace std;

namespace venom {
namespace analysis {

BoundFunction::BoundFunction(FuncSymbol* symbol,
                             const vector<InstantiatedType*>& params)
    : SymTypeParamsPair(symbol, params) {
  assert(symbol->getTypeParams().size() == params.size());
}

bool
BoundFunction::isFullyInstantiated() const {
  return InstantiatedType::IsFullyInstantiated(second.begin(), second.end());
}

bool
BoundFunction::isCodeGeneratable() const {
  return second.empty();
}

string
BoundFunction::createFuncName() const {
  stringstream buf;
  buf << first->getName();
  if (!second.empty()) {
    buf << "{";
    vector<string> names;
    names.reserve(second.size());
    for (vector<InstantiatedType*>::const_iterator it = second.begin();
         it != second.end(); ++it) {
      names.push_back((*it)->stringify());
    }
    buf << util::join(names.begin(), names.end(), ",");
    buf << "}";
  }
  return buf.str();
}

FuncSymbol*
BoundFunction::findSpecializedFuncSymbol() {
  InstantiatedType::AssertNoTypeParamPlaceholders(second);
  if (second.empty()) return first;
  SymbolTable* table = first->getDefinedSymbolTable();
  TypeTranslator t;
  FuncSymbol* specialized = table->findFuncSymbol(
      createFuncName(), SymbolTable::NoRecurse, t);
  VENOM_ASSERT_NOT_NULL(specialized);
  return specialized;
}

}
}
