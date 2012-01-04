#include <analysis/type.h>
#include <bootstrap/analysis.h>
#include <util/stl.h>

using namespace std;
using namespace venom::analysis;

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
  root->createClassSymbol("any", root->newChildScope(NULL), Type::AnyType);
  root->createClassSymbol("int", root->newChildScope(NULL), Type::IntType);
  root->createClassSymbol("bool", root->newChildScope(NULL), Type::BoolType);
  root->createClassSymbol("float", root->newChildScope(NULL), Type::FloatType);
  root->createClassSymbol("string", root->newChildScope(NULL), Type::StringType);
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
  objSymTab->createFuncSymbol("<ctor>", InstantiatedTypeVec(),
                              InstantiatedTypeVec(),
                              InstantiatedType::VoidType, true);
  root->createClassSymbol("object", objSymTab, Type::ObjectType);

  root->createClassSymbol("classtype", root->newChildScope(NULL), Type::ClassType,
                          createTypeParams(ctx, 1));

  root->createClassSymbol("list", root->newChildScope(NULL), Type::ListType,
                          createTypeParams(ctx, 1));
  root->createClassSymbol("map", root->newChildScope(NULL), Type::MapType,
                          createTypeParams(ctx, 2));

  // func symbols
  root->createFuncSymbol("print", InstantiatedTypeVec(),
                         util::vec1(InstantiatedType::AnyType),
                         InstantiatedType::VoidType, true);

  return root;
}

}
}
