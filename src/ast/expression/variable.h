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

  inline std::string& getName() { return name; }
  inline const std::string& getName() const { return name; }

  /** Returns NULL if no explicit type */
  inline const ParameterizedTypeString*
    getExplicitParameterizedTypeString() const { return explicit_type; }

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

  virtual analysis::BaseSymbol* getSymbol();

  virtual void registerSymbol(analysis::SemanticContext* ctx);

  virtual ASTNode* rewriteLocal(analysis::SemanticContext* ctx,
                                RewriteMode mode);

  virtual void codeGen(backend::CodeGenerator& cg);

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL(VariableNode)

protected:
  virtual analysis::InstantiatedType*
    typeCheckImpl(analysis::SemanticContext* ctx,
                  analysis::InstantiatedType* expected,
                  const analysis::InstantiatedTypeVec& typeParamArgs);

public:
  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(ident " << name;
    if (explicit_type) o << " " << *explicit_type;
    o << ")";
  }

private:
  ASTExpressionNode* createSymbolNode();

  std::string              name;
  ParameterizedTypeString* explicit_type;
};

class VariableSelfNode : public VariableNode {
public:
  VariableSelfNode()
    : VariableNode("self", NULL) {}

  virtual void registerSymbol(analysis::SemanticContext* ctx);

  virtual ASTNode* rewriteLocal(analysis::SemanticContext* ctx,
                                RewriteMode mode) {
    return NULL;
  }

  virtual void codeGen(backend::CodeGenerator& cg);

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL(VariableSelfNode)

protected:
  virtual analysis::InstantiatedType*
    typeCheckImpl(analysis::SemanticContext* ctx,
                  analysis::InstantiatedType* expected,
                  const analysis::InstantiatedTypeVec& typeParamArgs);

public:
  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(self)";
  }
};

class VariableSuperNode : public VariableNode {
public:
  VariableSuperNode()
    : VariableNode("super", NULL) {}

  virtual void registerSymbol(analysis::SemanticContext* ctx);

  virtual ASTNode* rewriteLocal(analysis::SemanticContext* ctx,
                                RewriteMode mode) {
    return NULL;
  }

protected:
  virtual analysis::InstantiatedType*
    typeCheckImpl(analysis::SemanticContext* ctx,
                  analysis::InstantiatedType* expected,
                  const analysis::InstantiatedTypeVec& typeParamArgs);

  virtual void codeGen(backend::CodeGenerator& cg);

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL(VariableSuperNode)

public:
  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(super)";
  }
};

}
}

#endif /* VENOM_AST_VARIABLE_H */
