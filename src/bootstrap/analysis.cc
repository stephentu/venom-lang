#include <analysis/type.h>
#include <bootstrap/analysis.h>
#include <util/stl.h>

using namespace venom::analysis;

namespace venom {
namespace bootstrap {

SymbolTable*
NewBootstrapSymbolTable(SemanticContext* ctx) {
  SymbolTable* root = new SymbolTable;
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

  root->createClassSymbol("func0", root->newChildScope(NULL), Type::Func0Type);
  root->createClassSymbol("func1", root->newChildScope(NULL), Type::Func1Type);
  root->createClassSymbol("func2", root->newChildScope(NULL), Type::Func2Type);
  root->createClassSymbol("func3", root->newChildScope(NULL), Type::Func3Type);
  root->createClassSymbol("func4", root->newChildScope(NULL), Type::Func4Type);
  root->createClassSymbol("func5", root->newChildScope(NULL), Type::Func5Type);
  root->createClassSymbol("func6", root->newChildScope(NULL), Type::Func6Type);
  root->createClassSymbol("func7", root->newChildScope(NULL), Type::Func7Type);
  root->createClassSymbol("func8", root->newChildScope(NULL), Type::Func8Type);
  root->createClassSymbol("func9", root->newChildScope(NULL), Type::Func9Type);

  root->createClassSymbol("func10", root->newChildScope(NULL), Type::Func10Type);
  root->createClassSymbol("func11", root->newChildScope(NULL), Type::Func11Type);
  root->createClassSymbol("func12", root->newChildScope(NULL), Type::Func12Type);
  root->createClassSymbol("func13", root->newChildScope(NULL), Type::Func13Type);
  root->createClassSymbol("func14", root->newChildScope(NULL), Type::Func14Type);
  root->createClassSymbol("func15", root->newChildScope(NULL), Type::Func15Type);
  root->createClassSymbol("func16", root->newChildScope(NULL), Type::Func16Type);
  root->createClassSymbol("func17", root->newChildScope(NULL), Type::Func17Type);
  root->createClassSymbol("func18", root->newChildScope(NULL), Type::Func18Type);
  root->createClassSymbol("func19", root->newChildScope(NULL), Type::Func19Type);

  root->createClassSymbol("object", root->newChildScope(NULL), Type::ObjectType);

  root->createClassSymbol("list", root->newChildScope(NULL), Type::ListType);
  root->createClassSymbol("map", root->newChildScope(NULL), Type::MapType);

  // func symbols
  root->createFuncSymbol("print",
                         util::vec1(InstantiatedType::AnyType),
                         InstantiatedType::VoidType);

  return root;
}

}
}
