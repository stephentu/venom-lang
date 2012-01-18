#include <analysis/type.h>

#include <bootstrap/analysis.h>

#include <runtime/box.h>
#include <runtime/builtin.h>
#include <runtime/venomobject.h>
#include <runtime/venomlist.h>
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
    string name = "T" + util::stringify(n);
    ret.push_back(ctx->createTypeParam(name, n)->instantiate(ctx));
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
  root->createClassSymbol("any", root->newChildScope(NULL), Type::AnyType);

  // primitives
  root->createClassSymbol("int", root->newChildScope(NULL), Type::IntType);
  root->createClassSymbol("bool", root->newChildScope(NULL), Type::BoolType);
  root->createClassSymbol("float", root->newChildScope(NULL), Type::FloatType);

  // void exists so we can give everything a return type
  root->createClassSymbol("void", root->newChildScope(NULL), Type::VoidType);

#define _CREATE_FUNC(n) \
  do { \
    root->createClassSymbol( \
        "func" #n, root->newChildScope(NULL), \
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

  SymbolTable *objSymTab = root->newChildScope(NULL);
  ClassSymbol *objectClassSym =
    root->createClassSymbol("object", objSymTab, Type::ObjectType);
  objSymTab->createMethodSymbol("<ctor>", InstantiatedTypeVec(),
                                InstantiatedTypeVec(),
                                InstantiatedType::VoidType,
                                objectClassSym, NULL, true);
  FuncSymbol *objStringifyFuncSym =
    objSymTab->createMethodSymbol("stringify", InstantiatedTypeVec(),
                                  InstantiatedTypeVec(),
                                  InstantiatedType::StringType,
                                  objectClassSym, NULL, true);

  SymbolTable *stringSymTab = root->newChildScope(NULL);
  ClassSymbol *stringClassSym =
    root->createClassSymbol("string", stringSymTab, Type::StringType);
  stringSymTab->createMethodSymbol("<ctor>", InstantiatedTypeVec(),
                                   InstantiatedTypeVec(),
                                   InstantiatedType::VoidType,
                                   stringClassSym, NULL, true);
  stringSymTab->createMethodSymbol("stringify", InstantiatedTypeVec(),
                                   InstantiatedTypeVec(),
                                   InstantiatedType::StringType,
                                   stringClassSym, objStringifyFuncSym, true);

  // boxed primitives, with hidden names
  SymbolTable *IntSymTab = root->newChildScope(NULL);
  ClassSymbol *IntClassSym =
    root->createClassSymbol("<Int>", IntSymTab, Type::BoxedIntType);
  IntSymTab->createMethodSymbol("<ctor>", InstantiatedTypeVec(),
                                util::vec1(InstantiatedType::IntType),
                                InstantiatedType::VoidType,
                                IntClassSym, NULL, true);

  SymbolTable *FloatSymTab = root->newChildScope(NULL);
  ClassSymbol *FloatClassSym =
    root->createClassSymbol("<Float>", FloatSymTab, Type::BoxedFloatType);
  FloatSymTab->createMethodSymbol("<ctor>", InstantiatedTypeVec(),
                                  util::vec1(InstantiatedType::FloatType),
                                  InstantiatedType::VoidType,
                                  FloatClassSym, NULL, true);

  SymbolTable *BoolSymTab = root->newChildScope(NULL);
  ClassSymbol *BoolClassSym =
    root->createClassSymbol("<Bool>", BoolSymTab, Type::BoxedBoolType);
  BoolSymTab->createMethodSymbol("<ctor>", InstantiatedTypeVec(),
                                 util::vec1(InstantiatedType::BoolType),
                                 InstantiatedType::VoidType,
                                 BoolClassSym, NULL, true);

  SymbolTable *RefSymTab = root->newChildScope(NULL);
  vector<InstantiatedType*> RefTypeParam = createTypeParams(ctx, 1);
  ClassSymbol *RefClassSym =
    root->createClassSymbol("<ref>", RefSymTab, Type::RefType, RefTypeParam);
  RefSymTab->createMethodSymbol("<ctor>", InstantiatedTypeVec(),
                                util::vec1(RefTypeParam[0]),
                                InstantiatedType::VoidType, RefClassSym,
                                NULL, true);
  RefSymTab->createClassAttributeSymbol("value", RefTypeParam[0], RefClassSym);

  root->createClassSymbol("classtype", root->newChildScope(NULL), Type::ClassType,
                          createTypeParams(ctx, 1));

  root->createClassSymbol("<moduletype>", root->newChildScope(NULL),
                          Type::ModuleType, InstantiatedTypeVec());

  SymbolTable *ListSymTab = root->newChildScope(NULL);
  vector<InstantiatedType*> ListTypeParam = createTypeParams(ctx, 1);
  ClassSymbol *ListClassSym =
    root->createClassSymbol("list", ListSymTab, Type::ListType, ListTypeParam);
  ListSymTab->createMethodSymbol("<ctor>", InstantiatedTypeVec(),
                                   InstantiatedTypeVec(),
                                   InstantiatedType::VoidType, ListClassSym,
                                   NULL, true);
  ListSymTab->createMethodSymbol("stringify", InstantiatedTypeVec(),
                                   InstantiatedTypeVec(),
                                   InstantiatedType::StringType, ListClassSym,
                                   objStringifyFuncSym, true);
  ListSymTab->createMethodSymbol("get", InstantiatedTypeVec(),
                                   util::vec1(InstantiatedType::IntType),
                                   ListTypeParam[0], ListClassSym,
                                   NULL, true);
  ListSymTab->createMethodSymbol("set", InstantiatedTypeVec(),
                                   util::vec2(InstantiatedType::IntType, ListTypeParam[0]),
                                   InstantiatedType::VoidType, ListClassSym,
                                   NULL, true);
  ListSymTab->createMethodSymbol("append", InstantiatedTypeVec(),
                                   util::vec1(ListTypeParam[0]),
                                   InstantiatedType::VoidType, ListClassSym,
                                   NULL, true);
  ListSymTab->createMethodSymbol("size", InstantiatedTypeVec(),
                                   InstantiatedTypeVec(),
                                   InstantiatedType::IntType, ListClassSym,
                                   NULL, true);

  root->createClassSymbol("map", root->newChildScope(NULL), Type::MapType,
                          createTypeParams(ctx, 2));

  // func symbols
  root->createFuncSymbol("print", InstantiatedTypeVec(),
                         util::vec1(InstantiatedType::AnyType),
                         InstantiatedType::VoidType, true);

  return root;
}

Linker::FuncDescMap
GetBuiltinFunctionMap(SemanticContext* rootCtx) {
  assert(rootCtx->isRootContext());
  Linker::FuncDescMap ret;

  // TODO: dynamically load this stuff, instead of hardcode
  ret["<prelude>.print"] = BuiltinPrintDescriptor;

  // object methods
  ret["<prelude>.object.<ctor>"]    = venom_object::CtorDescriptor;
  ret["<prelude>.object.stringify"] = venom_object::StringifyDescriptor;

  // string methods
  ret["<prelude>.string.<ctor>"]    = venom_string::CtorDescriptor;
  ret["<prelude>.string.stringify"] = venom_string::StringifyDescriptor;

  // box methods
  ret["<prelude>.<Int>.<ctor>"] = venom_integer::CtorDescriptor;
  ret["<prelude>.<Float>.<ctor>"] = venom_double::CtorDescriptor;
  ret["<prelude>.<Bool>.<ctor>"] = venom_boolean::CtorDescriptor;

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
        venom_list::ListType listType;
        if (arg0->isInt()) listType = venom_list::IntType;
        else if (arg0->isFloat()) listType = venom_list::FloatType;
        else if (arg0->isBool()) listType = venom_list::BoolType;
        else {
          assert(arg0->isRefCounted());
          listType = venom_list::RefType;
        }
        venom_class_object* classTable =
          venom_list::GetListClassTable(listType);

        // TODO: very hacky...
        string ctorName = scs->getFullName() + ".<ctor>";
        assert(ret.find(ctorName) == ret.end());
        ret[ctorName] = classTable->ctor;
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

  ret["<prelude>.object"] = &venom_object::ObjClassTable;
  ret["<prelude>.string"] = &venom_string::StringClassTable;

  ret["<prelude>.<Int>"]   = &venom_integer::IntegerClassTable;
  ret["<prelude>.<Float>"] = &venom_double::DoubleClassTable;
  ret["<prelude>.<Bool>"]  = &venom_boolean::BooleanClassTable;

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
        venom_list::ListType listType;
        if (arg0->isInt()) listType = venom_list::IntType;
        else if (arg0->isFloat()) listType = venom_list::FloatType;
        else if (arg0->isBool()) listType = venom_list::BoolType;
        else {
          assert(arg0->isRefCounted());
          listType = venom_list::RefType;
        }
        assert(ret.find(scs->getFullName()) == ret.end());
        ret[scs->getFullName()] = venom_list::GetListClassTable(listType);
      }
    }
  }
  return ret;
}

}
}
