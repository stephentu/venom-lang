#include <analysis/type.h>
#include <bootstrap/analysis.h>
#include <util/stl.h>

namespace venom {
namespace bootstrap {

analysis::SymbolTable*
NewBootstrapSymbolTable(analysis::SemanticContext* ctx) {
  analysis::SymbolTable* root = new analysis::SymbolTable;
  ctx->setRootSymbolTable(root);

  // types
  analysis::Type *any        = ctx->createType("any", NULL);
  analysis::Type *intType    = ctx->createType("int", any);
  analysis::Type *floatType  = ctx->createType("float", any);
  analysis::Type *stringType = ctx->createType("string", any);
  analysis::Type *voidType   = ctx->createType("void", any);
  analysis::Type *listType   = ctx->createType("list", any, 1);
  analysis::Type *mapType    = ctx->createType("map", any, 2);
  analysis::Type *objType    = ctx->createType("object", any);

  // class symbols
  root->createClassSymbol("any", any);
  root->createClassSymbol("int", intType);
  root->createClassSymbol("float", floatType);
  root->createClassSymbol("string", stringType);
  root->createClassSymbol("void", voidType);
  root->createClassSymbol("list", listType);
  root->createClassSymbol("map", mapType);
  root->createClassSymbol("object", objType);

  // func symbols
  root->createFuncSymbol("print",
                         util::vec1(any->instantiate(ctx)),
                         voidType->instantiate(ctx));

  return root;
}

}
}
