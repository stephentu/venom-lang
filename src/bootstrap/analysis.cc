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
  root->createClassSymbol("any", Type::AnyType);
  root->createClassSymbol("int", Type::IntType);
  root->createClassSymbol("bool", Type::BoolType);
  root->createClassSymbol("float", Type::FloatType);
  root->createClassSymbol("string", Type::StringType);
  root->createClassSymbol("void", Type::VoidType);
  root->createClassSymbol("object", Type::ObjectType);

  root->createClassSymbol("list", Type::ListType);
  root->createClassSymbol("map", Type::MapType);

  // func symbols
  root->createFuncSymbol("print",
                         util::vec1(InstantiatedType::AnyType),
                         InstantiatedType::VoidType);

  return root;
}

}
}
