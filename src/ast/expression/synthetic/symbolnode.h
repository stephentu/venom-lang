#ifndef VENOM_AST_SYMBOL_NODE_H
#define VENOM_AST_SYMBOL_NODE_H

#include <ast/expression/variable.h>

namespace venom {
namespace ast {

/**
 * Synthetic AST node
 */
class SymbolNode : public VariableNode {
public:
  /** Does *not* take ownership of symbol */
  SymbolNode(analysis::BaseSymbol* symbol,
             const analysis::TypeTranslator& translator =
                analysis::TypeTranslator(),
             analysis::InstantiatedType* explicitType =
                NULL);

  virtual analysis::InstantiatedType* getExplicitType()
    { return explicitType; }

  virtual void registerSymbol(analysis::SemanticContext* ctx) {}

  virtual void codeGen(backend::CodeGenerator& cg);

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL_EXPR(SymbolNode)

  virtual void print(std::ostream& o, size_t indent = 0);

private:
  analysis::InstantiatedType* explicitType;
};

}
}

#endif /* VENOM_AST_SYMBOL_NODE_H */
