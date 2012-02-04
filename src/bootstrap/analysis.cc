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

#include <analysis/type.h>

#include <bootstrap/analysis.h>

#include <runtime/box.h>
#include <runtime/builtin.h>

#include <runtime/venomobject.h>
#include <runtime/venomdict.h>
#include <runtime/venomlist.h>
#include <runtime/venomref.h>
#include <runtime/venomstring.h>

#include <util/stl.h>

using namespace std;
using namespace venom::analysis;
using namespace venom::backend;
using namespace venom::runtime;

namespace venom {
namespace bootstrap {

static inline vector<InstantiatedType*>
createTypeParams(SemanticContext* ctx, size_t n) {
  vector<InstantiatedType*> ret;
  ret.reserve(n);
  for (size_t i = 0; i < n; i++) {
    string name = "T" + util::stringify(i);
    ret.push_back(ctx->createTypeParam(name, i)->instantiate(ctx));
  }
  return ret;
}

SymbolTable*
NewBootstrapSymbolTable(SemanticContext* ctx) {
  SymbolTable* root = new SymbolTable(ctx);
  ctx->setRootSymbolTable(root);

  // class symbols
  // TODO: fill in child class scopes
  // TODO: passing NULL for the owner AST node is a hack for now
  //       we need to either build fake AST nodes, or create a new
  //       data structure to represent a scope
  // TODO: fill this information in dynamically- this is one of the most
  //       fragile parts of the system

  // any is the root of the class hierarchy. primitive types derive from any
  // assigning a primitive to an any type, however, causes boxing of the
  // primitive
  root->createClassSymbol("any", root->newChildScopeNoNode(), Type::AnyType);

  // primitives
  root->createClassSymbol("int", root->newChildScopeNoNode(), Type::IntType);
  root->createClassSymbol("bool", root->newChildScopeNoNode(), Type::BoolType);
  root->createClassSymbol("float", root->newChildScopeNoNode(), Type::FloatType);

  // void exists so we can give everything a return type
  root->createClassSymbol("void", root->newChildScopeNoNode(), Type::VoidType);

  // object must come before all its children
  SymbolTable* objSymTab = root->newChildScopeNoNode();
  ClassSymbol *objectClassSym =
    root->createClassSymbol("object", objSymTab, Type::ObjectType);
  objSymTab->createMethodSymbol("<ctor>", objSymTab->newChildScopeNoNode(),
                                InstantiatedTypeVec(),
                                InstantiatedTypeVec(),
                                InstantiatedType::VoidType,
                                objectClassSym, NULL, true);
  FuncSymbol *objStringifyFuncSym =
    objSymTab->createMethodSymbol("stringify", objSymTab->newChildScopeNoNode(),
                                  InstantiatedTypeVec(),
                                  InstantiatedTypeVec(),
                                  InstantiatedType::StringType,
                                  objectClassSym, NULL, true);
  FuncSymbol *objHashFuncSym =
    objSymTab->createMethodSymbol("hash", objSymTab->newChildScopeNoNode(),
                                  InstantiatedTypeVec(),
                                  InstantiatedTypeVec(),
                                  InstantiatedType::IntType,
                                  objectClassSym, NULL, true);

  // TODO: this eq is a hack for now- in the future, we will leave
  // eq out of object and require classes to extend
  // Equal{A} to be allowed as a key in a map. But our type-system
  // is currently not mature enough to support this.
  FuncSymbol *objEqFuncSym =
    objSymTab->createMethodSymbol("eq", objSymTab->newChildScopeNoNode(),
                                  InstantiatedTypeVec(),
                                  util::vec1(InstantiatedType::ObjectType),
                                  InstantiatedType::BoolType,
                                  objectClassSym, NULL, true);

#define _IMPL_OVERRIDE_STRINGIFY(classSym) \
  do { \
    (classSym)->getClassSymbolTable()->createMethodSymbol( \
        "stringify", (classSym)->getClassSymbolTable()->newChildScopeNoNode(), \
        InstantiatedTypeVec(), \
        InstantiatedTypeVec(), \
        InstantiatedType::StringType, \
        (classSym), objStringifyFuncSym, true); \
  } while (0)

#define _IMPL_OVERRIDE_HASH(classSym) \
  do { \
    (classSym)->getClassSymbolTable()->createMethodSymbol( \
        "hash", (classSym)->getClassSymbolTable()->newChildScopeNoNode(), \
        InstantiatedTypeVec(), \
        InstantiatedTypeVec(), \
        InstantiatedType::IntType, \
        (classSym), objHashFuncSym, true); \
  } while (0)

#define _IMPL_OVERRIDE_EQ(classSym) \
  do { \
    (classSym)->getClassSymbolTable()->createMethodSymbol( \
        "eq", (classSym)->getClassSymbolTable()->newChildScopeNoNode(), \
        InstantiatedTypeVec(), \
        util::vec1(InstantiatedType::ObjectType), \
        InstantiatedType::BoolType, \
        (classSym), objEqFuncSym, true); \
  } while (0)

#define _IMPL_OVERRIDE_ALL(classSym) \
  do { \
    _IMPL_OVERRIDE_STRINGIFY(classSym); \
    _IMPL_OVERRIDE_HASH(classSym); \
    _IMPL_OVERRIDE_EQ(classSym); \
  } while (0)

#define _CREATE_FUNC(n) \
  do { \
    root->createClassSymbol( \
        "func" #n, root->newChildScopeNoNode(), \
        Type::Func ## n ## Type, createTypeParams(ctx, 1 + n)); \
  } while (0)

  _CREATE_FUNC(0);
  _CREATE_FUNC(1);
  _CREATE_FUNC(2);
  _CREATE_FUNC(3);
  _CREATE_FUNC(4);
  _CREATE_FUNC(5);
  _CREATE_FUNC(6);
  _CREATE_FUNC(7);
  _CREATE_FUNC(8);
  _CREATE_FUNC(9);

  _CREATE_FUNC(10);
  _CREATE_FUNC(11);
  _CREATE_FUNC(12);
  _CREATE_FUNC(13);
  _CREATE_FUNC(14);
  _CREATE_FUNC(15);
  _CREATE_FUNC(16);
  _CREATE_FUNC(17);
  _CREATE_FUNC(18);
  _CREATE_FUNC(19);

#undef _CREATE_FUNC

  SymbolTable *stringSymTab = root->newChildScopeNoNode();
  ClassSymbol *stringClassSym =
    root->createClassSymbol("string", stringSymTab, Type::StringType);
  stringSymTab->createMethodSymbol("<ctor>", stringSymTab->newChildScopeNoNode(),
                                   InstantiatedTypeVec(),
                                   InstantiatedTypeVec(),
                                   InstantiatedType::VoidType,
                                   stringClassSym, NULL, true);

  _IMPL_OVERRIDE_ALL(stringClassSym);

  stringSymTab->createMethodSymbol("concat", stringSymTab->newChildScopeNoNode(),
                                   InstantiatedTypeVec(),
                                   util::vec1(InstantiatedType::StringType),
                                   InstantiatedType::StringType, stringClassSym,
                                   NULL, true);

  // boxed primitives, with hidden names
  SymbolTable *IntSymTab = root->newChildScopeNoNode();
  ClassSymbol *IntClassSym =
    root->createClassSymbol("<Int>", IntSymTab, Type::BoxedIntType);
  IntSymTab->createMethodSymbol("<ctor>", IntSymTab->newChildScopeNoNode(),
                                InstantiatedTypeVec(),
                                util::vec1(InstantiatedType::IntType),
                                InstantiatedType::VoidType,
                                IntClassSym, NULL, true);
  _IMPL_OVERRIDE_ALL(IntClassSym);

  SymbolTable *FloatSymTab = root->newChildScopeNoNode();
  ClassSymbol *FloatClassSym =
    root->createClassSymbol("<Float>", FloatSymTab, Type::BoxedFloatType);
  FloatSymTab->createMethodSymbol("<ctor>", FloatSymTab->newChildScopeNoNode(),
                                  InstantiatedTypeVec(),
                                  util::vec1(InstantiatedType::FloatType),
                                  InstantiatedType::VoidType,
                                  FloatClassSym, NULL, true);
  _IMPL_OVERRIDE_ALL(FloatClassSym);

  SymbolTable *BoolSymTab = root->newChildScopeNoNode();
  ClassSymbol *BoolClassSym =
    root->createClassSymbol("<Bool>", BoolSymTab, Type::BoxedBoolType);
  BoolSymTab->createMethodSymbol("<ctor>", BoolSymTab->newChildScopeNoNode(),
                                 InstantiatedTypeVec(),
                                 util::vec1(InstantiatedType::BoolType),
                                 InstantiatedType::VoidType,
                                 BoolClassSym, NULL, true);
  _IMPL_OVERRIDE_ALL(BoolClassSym);

  SymbolTable *RefSymTab = root->newChildScopeNoNode();
  vector<InstantiatedType*> RefTypeParam = createTypeParams(ctx, 1);
  ClassSymbol *RefClassSym =
    root->createClassSymbol("<ref>", RefSymTab, Type::RefType, RefTypeParam);
  RefSymTab->createMethodSymbol("<ctor>", RefSymTab->newChildScopeNoNode(),
                                InstantiatedTypeVec(),
                                InstantiatedTypeVec(),
                                InstantiatedType::VoidType, RefClassSym,
                                NULL, true);
  RefSymTab->createClassAttributeSymbol("value", RefTypeParam[0], RefClassSym);

  root->createClassSymbol("classtype", root->newChildScopeNoNode(), Type::ClassType,
                          createTypeParams(ctx, 1));

  root->createClassSymbol("<moduletype>", root->newChildScopeNoNode(),
                          Type::ModuleType, InstantiatedTypeVec());

  SymbolTable *ListSymTab = root->newChildScopeNoNode();
  vector<InstantiatedType*> ListTypeParam = createTypeParams(ctx, 1);
  ClassSymbol *ListClassSym =
    root->createClassSymbol("list", ListSymTab, Type::ListType, ListTypeParam);
  ListSymTab->createMethodSymbol("<ctor>", ListSymTab->newChildScopeNoNode(),
                                 InstantiatedTypeVec(),
                                 InstantiatedTypeVec(),
                                 InstantiatedType::VoidType, ListClassSym,
                                 NULL, true);

  _IMPL_OVERRIDE_ALL(ListClassSym);

  ListSymTab->createMethodSymbol("get", ListSymTab->newChildScopeNoNode(),
                                 InstantiatedTypeVec(),
                                 util::vec1(InstantiatedType::IntType),
                                 ListTypeParam[0], ListClassSym,
                                 NULL, true);
  ListSymTab->createMethodSymbol("set", ListSymTab->newChildScopeNoNode(),
                                 InstantiatedTypeVec(),
                                 util::vec2(InstantiatedType::IntType, ListTypeParam[0]),
                                 InstantiatedType::VoidType, ListClassSym,
                                 NULL, true);
  ListSymTab->createMethodSymbol("append", ListSymTab->newChildScopeNoNode(),
                                 InstantiatedTypeVec(),
                                 util::vec1(ListTypeParam[0]),
                                 InstantiatedType::VoidType, ListClassSym,
                                 NULL, true);
  ListSymTab->createMethodSymbol("size", ListSymTab->newChildScopeNoNode(),
                                 InstantiatedTypeVec(),
                                 InstantiatedTypeVec(),
                                 InstantiatedType::IntType, ListClassSym,
                                 NULL, true);

  SymbolTable *MapSymTab = root->newChildScopeNoNode();
  vector<InstantiatedType*> MapTypeParams = createTypeParams(ctx, 2);
  ClassSymbol *MapClassSym =
    root->createClassSymbol("map", MapSymTab, Type::MapType, MapTypeParams);
  MapSymTab->createMethodSymbol("<ctor>", MapSymTab->newChildScopeNoNode(),
                                InstantiatedTypeVec(),
                                InstantiatedTypeVec(),
                                InstantiatedType::VoidType, MapClassSym,
                                NULL, true);

  _IMPL_OVERRIDE_ALL(MapClassSym);

  MapSymTab->createMethodSymbol("get", MapSymTab->newChildScopeNoNode(),
                                InstantiatedTypeVec(),
                                util::vec1(MapTypeParams[0]),
                                MapTypeParams[1], MapClassSym,
                                NULL, true);
  MapSymTab->createMethodSymbol("set", MapSymTab->newChildScopeNoNode(),
                                InstantiatedTypeVec(),
                                MapTypeParams,
                                InstantiatedType::VoidType, MapClassSym,
                                NULL, true);
  MapSymTab->createMethodSymbol("size", MapSymTab->newChildScopeNoNode(),
                                InstantiatedTypeVec(),
                                InstantiatedTypeVec(),
                                InstantiatedType::IntType, MapClassSym,
                                NULL, true);

  // func symbols
  root->createFuncSymbol("print", root->newChildScopeNoNode(),
                         InstantiatedTypeVec(),
                         util::vec1(InstantiatedType::AnyType),
                         InstantiatedType::VoidType, true);

#undef _IMPL_OVERRIDE_STRINGIFY
#undef _IMPL_OVERRIDE_HASH
#undef _IMPL_OVERRIDE_EQ
#undef _IMPL_OVERRIDE_ALL

  return root;
}

static void FillFunctionMap(
    Linker::FuncDescMap& map,
    ClassSymbol* csym,
    venom_class_object* classObj) {
  map[csym->getFullName() + ".<ctor>"] = classObj->ctor;

  vector<Symbol*> attributes;
  vector<FuncSymbol*> methods;
  csym->linearizedOrder(attributes, methods);
  assert(methods.size() == classObj->vtable.size());
  for (size_t idx = 0; idx < methods.size(); idx++) {
    pair<Linker::FuncDescMap::iterator, bool> res =
      map.insert(
          make_pair(
            methods[idx]->getFullName(), classObj->vtable[idx]));
    if (!res.second) {
      // existing element
      assert( res.first->second == classObj->vtable[idx] );

    }
  }
}

static inline venom_cell::CellType
TypeToCellType(InstantiatedType* type) {
  venom_cell::CellType retType;
  if (type->isInt())        retType = venom_cell::IntType;
  else if (type->isFloat()) retType = venom_cell::FloatType;
  else if (type->isBool())  retType = venom_cell::BoolType;
  else {
    assert(type->isRefCounted());
    retType = venom_cell::RefType;
  }
  return retType;
}

Linker::FuncDescMap
GetBuiltinFunctionMap(SemanticContext* rootCtx) {
  assert(rootCtx->isRootContext());
  Linker::FuncDescMap ret;

  // TODO: dynamically load this stuff, instead of hardcode
  ret["<prelude>.print"] = &BuiltinPrintDescriptor();

  // object methods
  FillFunctionMap(
      ret, Type::ObjectType->getClassSymbol(),
      &venom_object::ObjClassTable());

  // string methods
  FillFunctionMap(
      ret, Type::StringType->getClassSymbol(),
      &venom_string::StringClassTable());

  // box methods
  FillFunctionMap(
      ret, Type::BoxedIntType->getClassSymbol(),
      &venom_integer::IntegerClassTable());
  FillFunctionMap(
      ret, Type::BoxedFloatType->getClassSymbol(),
      &venom_double::DoubleClassTable());
  FillFunctionMap(
      ret, Type::BoxedBoolType->getClassSymbol(),
      &venom_boolean::BooleanClassTable());

  vector<ClassSymbol*> builtinClassSyms;
  rootCtx->getRootSymbolTable()->getClassSymbols(builtinClassSyms);

  for (vector<ClassSymbol*>::iterator it = builtinClassSyms.begin();
       it != builtinClassSyms.end(); ++it) {
    if (SpecializedClassSymbol* scs =
          dynamic_cast<SpecializedClassSymbol*>(*it)) {
      InstantiatedType* itype = scs->getInstantiation();
      // list
      if (itype->getType()->isListType()) {
        assert(itype->getParams().size() == 1);
        InstantiatedType* arg0 = itype->getParams()[0];
        venom_class_object* classTable =
          venom_list::GetListClassTable(TypeToCellType(arg0));
        FillFunctionMap(
            ret, scs->getType()->getClassSymbol(), classTable);
      } else if (itype->getType()->isMapType()) {
        assert(itype->getParams().size() == 2);
        InstantiatedType* keyType = itype->getParams()[0];
        InstantiatedType* valueType = itype->getParams()[1];
        venom_class_object* classTable =
          venom_dict::GetDictClassTable(
              TypeToCellType(keyType), TypeToCellType(valueType));
        FillFunctionMap(
            ret, scs->getType()->getClassSymbol(), classTable);
      } else if (itype->getType()->isRefType()) {
        assert(itype->getParams().size() == 1);
        FillFunctionMap(
            ret, scs->getType()->getClassSymbol(),
            venom_ref::GetRefClassTable(
              itype->getParams()[0]->isRefCounted()));
      }
    }
  }

  return ret;
}

Linker::ClassObjMap
GetBuiltinClassMap(SemanticContext* rootCtx) {
  assert(rootCtx->isRootContext());
  Linker::ClassObjMap ret;
  // TODO: dynamically load this stuff, instead of hardcode

  ret["<prelude>.object"] = &venom_object::ObjClassTable();
  ret["<prelude>.string"] = &venom_string::StringClassTable();

  ret["<prelude>.<Int>"]   = &venom_integer::IntegerClassTable();
  ret["<prelude>.<Float>"] = &venom_double::DoubleClassTable();
  ret["<prelude>.<Bool>"]  = &venom_boolean::BooleanClassTable();

  vector<ClassSymbol*> builtinClassSyms;
  rootCtx->getRootSymbolTable()->getClassSymbols(builtinClassSyms);

  for (vector<ClassSymbol*>::iterator it = builtinClassSyms.begin();
       it != builtinClassSyms.end(); ++it) {
    if (SpecializedClassSymbol* scs =
          dynamic_cast<SpecializedClassSymbol*>(*it)) {
      InstantiatedType* itype = scs->getInstantiation();
      // list
      if (itype->getType()->isListType()) {
        assert(itype->getParams().size() == 1);
        InstantiatedType* arg0 = itype->getParams()[0];
        assert(ret.find(scs->getFullName()) == ret.end());
        ret[scs->getFullName()] =
          venom_list::GetListClassTable(TypeToCellType(arg0));
      } else if (itype->getType()->isMapType()) {
        assert(itype->getParams().size() == 2);
        InstantiatedType* keyType = itype->getParams()[0];
        InstantiatedType* valueType = itype->getParams()[1];
        assert(ret.find(scs->getFullName()) == ret.end());
        ret[scs->getFullName()] =
          venom_dict::GetDictClassTable(
              TypeToCellType(keyType), TypeToCellType(valueType));
      } else if (itype->getType()->isRefType()) {
        assert(itype->getParams().size() == 1);
        assert(ret.find(scs->getFullName()) == ret.end());
        ret[scs->getFullName()] =
          venom_ref::GetRefClassTable(
              itype->getParams()[0]->isRefCounted());
      }
    }
  }
  return ret;
}

}
}
