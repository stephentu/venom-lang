#include <algorithm>
#include <cassert>

#include <analysis/semanticcontext.h>
#include <analysis/symboltable.h>

#include <ast/expression/node.h>
#include <ast/statement/classdecl.h>

using namespace std;
using namespace venom::ast;

namespace venom {
namespace analysis {

struct functor {
  functor(SemanticContext* ctx, TypeTranslator* t) : ctx(ctx), t(t) {}
  inline InstantiatedType* operator()(InstantiatedType* type) const {
    return t->translate(ctx, type);
  }
  SemanticContext* ctx;
  TypeTranslator*  t;
};

struct find_functor {
  find_functor(InstantiatedType* type) : type(type) {}
  inline bool operator()(const InstantiatedTypePair& p) const {
    return p.first->equals(*type);
  }
  InstantiatedType* type;
};

InstantiatedType*
TypeTranslator::translate(SemanticContext* ctx, InstantiatedType* type) {
  TypeMap::iterator it =
    find_if(map.begin(), map.end(), find_functor(type));
  if (it != map.end()) return it->second;
  vector<InstantiatedType*> buf(type->getParams().size());
  transform(type->getParams().begin(), type->getParams().end(),
            buf.begin(), functor(ctx, this));
  return ctx->createInstantiatedType(type->getType(), buf);
}

SymbolTable::SymbolTable(SymbolTable* parent, const TypeMap& map, ASTNode* owner)
  : owner(owner),
    symbolContainer(
        parent ?
          util::vec1(&parent->symbolContainer) :
          CType<Symbol*>::vec_type(),
        parent ?
          util::vec1(map) :
          vector<TypeMap>()),
    funcContainer(
        parent ?
          util::vec1(&parent->funcContainer) :
          CType<FuncSymbol*>::vec_type(),
        parent ?
          util::vec1(map) :
          vector<TypeMap>()),
    classContainer(
        parent ?
          util::vec1(&parent->classContainer) :
          CType<ClassSymbol*>::vec_type(),
        parent ?
          util::vec1(map) :
          vector<TypeMap>()) {
  assert(parent || map.empty());
}

bool
SymbolTable::isDefined(const string& name, unsigned int type, bool recurse) {
  TypeTranslator t;
  return findBaseSymbol(name, type, recurse, t);
}

BaseSymbol*
SymbolTable::findBaseSymbol(const string& name, unsigned int type, bool recurse,
                            TypeTranslator& translator) {
  assert(type & (Location | Function | Class));
  BaseSymbol *ret = NULL;
  if ((type & Location) && (ret = findSymbol(name, recurse, translator)))
    return ret;
  if ((type & Function) && (ret = findFuncSymbol(name, recurse, translator)))
    return ret;
  if ((type & Class) && (ret = findClassSymbol(name, recurse, translator)))
    return ret;
  return NULL;
}

Symbol*
SymbolTable::createSymbol(const string&     name,
                          InstantiatedType* type) {
  Symbol *sym = new Symbol(name, this, type);
  symbolContainer.insert(name, sym);
  return sym;
}

Symbol*
SymbolTable::findSymbol(const string& name, bool recurse,
                        TypeTranslator& translator) {
  Symbol *ret = NULL;
  symbolContainer.find(ret, name, recurse, translator);
  return ret;
}

FuncSymbol*
SymbolTable::createFuncSymbol(const string&                    name,
                              const vector<InstantiatedType*>& params,
                              InstantiatedType*                returnType) {
  FuncSymbol *sym = new FuncSymbol(name, this, params, returnType);
  funcContainer.insert(name, sym);
  return sym;
}

FuncSymbol*
SymbolTable::findFuncSymbol(const string& name, bool recurse,
                            TypeTranslator& translator) {
  FuncSymbol *ret = NULL;
  funcContainer.find(ret, name, recurse, translator);
  return ret;
}

ClassSymbol*
SymbolTable::createClassSymbol(const string& name,
                               SymbolTable*  classTable,
                               Type*         type) {
  ClassSymbol *sym = new ClassSymbol(name, this, classTable, type);
  type->setClassSymbol(sym);
  classContainer.insert(name, sym);
  return sym;
}

ClassSymbol*
SymbolTable::findClassSymbol(const string& name, bool recurse,
                             TypeTranslator& translator) {
  ClassSymbol *ret = NULL;
  classContainer.find(ret, name, recurse, translator);
  return ret;
}

//ClassSymbol*
//SymbolTable::findClassSymbol(const ParameterizedTypeString* name,
//                             bool recurse,
//                             const ParameterizedTypeString*& failed_type,
//                             bool& wrong_params) {
//  failed_type = NULL; wrong_params = false;
//
//  ClassSymbol *ret = findClassSymbol(name->name, recurse);
//  if (ret) {
//    // num param check
//    if (name->params.size() != ret->getType()->getParams()) {
//      failed_type = name;
//      wrong_params = true;
//      return NULL;
//    }
//    bool fail = false;
//    for (TypeStringVec::const_iterator it = name->params.begin();
//         it != name->params.end(); ++it) {
//      ClassSymbol *cs = findClassSymbol(*it, true, failed_type, wrong_params);
//      if (!cs) {
//        assert(failed_type);
//        fail = true;
//        break;
//      }
//    }
//    return fail ? NULL : ret;
//  } else {
//    failed_type = name;
//    return NULL;
//  }
//}
//
//ClassSymbol*
//SymbolTable::findClassSymbolOrThrow(const ParameterizedTypeString* type,
//                                    bool recurse) {
//  const ParameterizedTypeString* failed_type;
//  bool wrong_params;
//  ClassSymbol *parsym = findClassSymbol(type, true, failed_type, wrong_params);
//  if (!parsym) {
//    assert(failed_type);
//    if (wrong_params) {
//      throw SemanticViolationException(
//          "Wrong number of type parameters given to " + failed_type->name);
//    } else {
//      throw SemanticViolationException(
//          "Type " + failed_type->name + " not defined");
//    }
//  }
//  return parsym;
//}

}
}
