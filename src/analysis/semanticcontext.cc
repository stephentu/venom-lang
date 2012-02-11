/**
 * Copyright (c) 2012 Stephen Tu
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names
 * of its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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

bool
SemanticContext::isDescendantOf(const SemanticContext* that) const {
  const SemanticContext* cur = this;
  while (cur && cur != that) cur = cur->parent;
  return cur == that;
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
  child->setRootSymbolTable(rootSymbols->newChildScope(child));
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
  SymbolTable *cur = type->getStartingScope() ?
    type->getStartingScope() : symbols;
  BaseSymbol *bs = NULL;
  for (vector<string>::const_iterator it = type->getNames().begin();
       it != type->getNames().end(); ++it) {
    bs = cur->findBaseSymbol(
        *it,
        (SymbolTable::Class | SymbolTable::Module),
        (it == type->getNames().begin()) ?
            SymbolTable::AllowCurrentScope :
            SymbolTable::ClassLookup,
        t);
    if (!bs) {
      throw SemanticViolationException(
          "Type/Module " + util::join(type->getNames().begin(), it + 1, ".") +
          " not defined");
    }
    if (ClassSymbol *cs = dynamic_cast<ClassSymbol*>(bs)) {
      if (it != type->getNames().end() - 1) {
        // TODO: fix this limitation
        // for now, only the outermost type can have type parameters
        if (cs->getType()->getParams()) {
          throw SemanticViolationException(
              "Implementation limitation: Cannot select inner class "
              "from parameterized class: " +
              util::join(type->getNames().begin(), it + 1, "."));
        }
      }
      cur = cs->getClassSymbolTable();
    } else if (ModuleSymbol *ms = dynamic_cast<ModuleSymbol*>(bs)) {
      if (this != ms->getOriginalContext()) {
        throw TypeViolationException(
            "Cannot access imported modules of another module");
      }
      cur = ms->getModuleSymbolTable();
      if (it == type->getNames().end() - 1) {
        throw SemanticViolationException(
            "Cannot name module " + ms->getName() + " as type");
      }
    } else assert(false);
    assert(cur);
  }
  assert(bs);
  ClassSymbol *cs = static_cast<ClassSymbol*>(bs);
  vector<InstantiatedType*> buf(type->getParams().size());
  transform(type->getParams().begin(), type->getParams().end(),
            buf.begin(), InstantiateFunctor(this, symbols));

  return t.translate(this, cs->getType()->instantiate(this, buf));
}

}
}
