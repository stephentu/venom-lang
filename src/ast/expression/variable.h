#ifndef VENOM_AST_VARIABLE_H
#define VENOM_AST_VARIABLE_H

#include <stdexcept>
#include <string>
#include <vector>

#include <ast/expression/node.h>

namespace venom {
namespace ast {

class VariableNode : public ASTExpressionNode {
public:
  /** Takes ownership of explicit_type */
  VariableNode(const std::string&       name,
               ParameterizedTypeString* explicit_type)
    : name(name), explicit_type(explicit_type) {}

  ~VariableNode() {
    if (explicit_type) delete explicit_type;
  }

  /** Returns NULL if no explicit type */
  inline const ParameterizedTypeString*
  getExplicitParameterizedTypeString() const { return explicit_type; }

  virtual size_t getNumKids() const { return 0; }

  virtual ASTNode* getNthKid(size_t kid) {
    throw std::out_of_range(__func__);
  }

  virtual bool needsNewScope(size_t k) const { return false; }

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(ident " << name;
    if (explicit_type) o << " " << *explicit_type;
    o << ")";
  }

private:
  std::string              name;
  ParameterizedTypeString* explicit_type;
};

class VariableSelfNode : public VariableNode {
public:
  VariableSelfNode()
    : VariableNode("self", NULL) {}

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(self)";
  }
};

}
}

#endif /* VENOM_AST_VARIABLE_H */
