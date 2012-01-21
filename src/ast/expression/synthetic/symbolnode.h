#ifndef VENOM_AST_SYMBOL_NODE_H
#define VENOM_AST_SYMBOL_NODE_H

#include <ast/expression/node.h>

namespace venom {
namespace ast {

/**
 * Synthetic AST node
 */
class SymbolNode : public ASTExpressionNode {
public:
  /** Does *not* take ownership of symbol, nor type */
  SymbolNode(analysis::BaseSymbol* symbol,
             analysis::InstantiatedType* staticType,
             analysis::InstantiatedType* expectedType)
    : symbol(symbol) {
    this->staticType = staticType;
    this->expectedType = expectedType;
  }

  virtual size_t getNumKids() const { return 0; }

  virtual ASTNode* getNthKid(size_t kid) {
    throw std::out_of_range(VENOM_SOURCE_INFO);
  }

  virtual void setNthKid(size_t idx, ASTNode* kid) {
    throw std::out_of_range(VENOM_SOURCE_INFO);
  }

  virtual bool needsNewScope(size_t k) const {
    throw std::out_of_range(VENOM_SOURCE_INFO);
  }

  virtual analysis::BaseSymbol* getSymbol() { return symbol; }

  virtual void codeGen(backend::CodeGenerator& cg);

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL_EXPR(SymbolNode)

protected:
  virtual analysis::InstantiatedType*
    typeCheckImpl(analysis::SemanticContext* ctx,
                  analysis::InstantiatedType* expected,
                  const analysis::InstantiatedTypeVec& typeParamArgs);

public:
  virtual void print(std::ostream& o, size_t indent = 0);

private:
  analysis::BaseSymbol* symbol;
};

}
}

#endif /* VENOM_AST_SYMBOL_NODE_H */
