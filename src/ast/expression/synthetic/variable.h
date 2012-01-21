#ifndef VENOM_AST_SYNTHETIC_VARIABLE_H
#define VENOM_AST_SYNTHETIC_VARIABLE_H

#include <cassert>
#include <ast/expression/variable.h>

namespace venom {
namespace ast {

class VariableNodeSynthetic : public VariableNode {
public:
  VariableNodeSynthetic(const std::string& name,
                        analysis::InstantiatedType* explicitType)
    : VariableNode(name),
      explicitType(explicitType) { assert(explicitType); }

  virtual analysis::InstantiatedType* getExplicitType()
    { return explicitType; }

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL_EXPR(VariableNode)

public:
  virtual void print(std::ostream& o, size_t indent = 0);

private:
  analysis::InstantiatedType* explicitType;
};

}
}

#endif /* VENOM_AST_SYNTHETIC_VARIABLE_H */
