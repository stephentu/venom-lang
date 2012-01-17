#include <algorithm>

#include <analysis/semanticcontext.h>
#include <ast/expression/node.h>
#include <ast/statement/node.h>
#include <backend/linker.h>

using namespace std;
using namespace venom::ast;
using namespace venom::backend;

namespace venom {
namespace analysis {

SemanticContext::~SemanticContext() {
  // we have ownership of:
  // moduleRoot + objectCode + children + types + rootSymbols
  if (moduleRoot) delete moduleRoot;
  if (objectCode) delete objectCode;
  util::delete_pointers(children.begin(), children.end());
  util::delete_pointers(types.begin(), types.end());
  if (!parent) {
    delete rootSymbols;
    Type::ResetBuiltinTypes();
  }
}

string SemanticContext::getFullModuleName() const {
  // TODO: implement me
  return moduleName;
}

void SemanticContext::collectObjectCode(vector<ObjectCode*>& objCodes) {
  if (objectCode) objCodes.push_back(objectCode);
  for (vector<SemanticContext*>::iterator it = children.begin();
       it != children.end(); ++it) {
    (*it)->collectObjectCode(objCodes);
  }
}

SemanticContext*
SemanticContext::newChildContext(const string& moduleName) {
  SemanticContext *child =
    new SemanticContext(moduleName, this,
                        isRootContext() ? this : programRoot);
  child->setRootSymbolTable(rootSymbols->newChildScope(child, NULL));
  childrenMap[moduleName] = child;
  children.push_back(child);
  return child;
}

SemanticContext* SemanticContext::findModule(const util::StrVec& names) {
  if (names.empty()) return this;
  map<string, SemanticContext*>::iterator it = childrenMap.find(names[0]);
  if (it == childrenMap.end()) return NULL;
  // TODO: might want to use iterators for this, internally
  return it->second->findModule(util::StrVec(names.begin() + 1, names.end()));
}

SemanticContext* SemanticContext::createModule(const util::StrVec& names) {
  if (names.empty()) return this;
  map<string, SemanticContext*>::iterator it = childrenMap.find(names[0]);
  if (it == childrenMap.end()) {
    // make a new child
    SemanticContext *child = newChildContext(names[0]);
    return child->createModule(util::StrVec(names.begin() + 1, names.end()));
  }
  // TODO: might want to use iterators for this, internally
  return it->second->createModule(
      util::StrVec(names.begin() + 1, names.end()));
}

Type* SemanticContext::createType(const string&     name,
                                  InstantiatedType* parent,
                                  size_t            params /* = 0*/) {
  Type* t = new Type(name, NULL, parent, params);
  types.push_back(t);
  return t;
}

Type* SemanticContext::createTypeParam(const string& name, size_t pos) {
  Type* t = new TypeParamType(name, pos);
  types.push_back(t);
  return t;
}

Type*
SemanticContext::createModuleType(const string& name) {
  return createType(name + "$$<module>", InstantiatedType::ModuleType, 0);
}

InstantiatedType*
SemanticContext::createInstantiatedType(
    Type* type, const vector<InstantiatedType*>& params) {
  InstantiatedType* t = new InstantiatedType(type, params);
  itypes.push_back(t);
  return t;
}

InstantiatedType*
SemanticContext::instantiateOrThrow(SymbolTable *symbols,
                                    const ParameterizedTypeString* type) {

  TypeTranslator t;
  SymbolTable *cur = symbols;
  BaseSymbol *bs = NULL;
  for (vector<string>::const_iterator it = type->names.begin();
       it != type->names.end(); ++it) {
    bs = cur->findBaseSymbol(
        *it,
        SymbolTable::Class | SymbolTable::Module,
        it == type->names.begin() ?
            SymbolTable::AllowCurrentScope : SymbolTable::ClassLookup,
        t);
    if (!bs) {
      throw SemanticViolationException(
          "Type/Module " + util::join(type->names.begin(), it + 1, ".") +
          " not defined");
    }
    if (ClassSymbol *cs = dynamic_cast<ClassSymbol*>(bs)) {
      if (it != type->names.end() - 1) {
        // TODO: fix this limitation
        // for now, only the outermost type can have type parameters
        if (cs->getType()->getParams()) {
          throw SemanticViolationException(
              "Implementation limitation: Cannot select inner class "
              "from parameterized class: " +
              util::join(type->names.begin(), it + 1, "."));
        }
      }
      cur = cs->getClassSymbolTable();
    } else if (ModuleSymbol *ms = dynamic_cast<ModuleSymbol*>(bs)) {
      if (this != ms->getOriginalContext()) {
        throw TypeViolationException(
            "Cannot access imported modules of another module");
      }
      cur = ms->getModuleSymbolTable();
      if (it == type->names.end() - 1) {
        throw SemanticViolationException(
            "Cannot name module " + ms->getName() + " as type");
      }
    } else assert(false);
    assert(cur);
  }
  assert(bs);
  ClassSymbol *cs = static_cast<ClassSymbol*>(bs);
  vector<InstantiatedType*> buf(type->params.size());
  transform(type->params.begin(), type->params.end(),
            buf.begin(), InstantiateFunctor(this, symbols));

  return t.translate(this, cs->getType()->instantiate(this, buf));
}

}
}
