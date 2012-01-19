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
  VariableNode(const std::string& name)
    : name(name) {}

  inline std::string& getName() { return name; }
  inline const std::string& getName() const { return name; }

  /** Returns NULL if no explicit type */
  virtual analysis::InstantiatedType* getExplicitType() = 0;

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

  VENOM_AST_TYPED_CLONE(VariableNode)

protected:
  virtual analysis::InstantiatedType*
    typeCheckImpl(analysis::SemanticContext* ctx,
                  analysis::InstantiatedType* expected,
                  const analysis::InstantiatedTypeVec& typeParamArgs);

  std::string name;
};

class VariableNodeParser : public VariableNode {
public:
  /** Takes ownership of explicitTypeString */
  VariableNodeParser(const std::string&       name,
                     ParameterizedTypeString* explicitTypeString)
    : VariableNode(name),
      explicitTypeString(explicitTypeString),
      explicitType(NULL) {}

  ~VariableNodeParser() {
    if (explicitTypeString) delete explicitTypeString;
  }

  inline const ParameterizedTypeString*
    getExplicitParameterizedTypeString() const { return explicitTypeString; }

  virtual analysis::InstantiatedType* getExplicitType();

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL(VariableNode)

public:
  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(ident " << name;
    if (explicitTypeString) o << " " << *explicitTypeString;
    o << ")";
  }

private:
  ParameterizedTypeString* explicitTypeString;
  analysis::InstantiatedType* explicitType;
};

class VariableSelfNode : public VariableNodeParser {
public:
  VariableSelfNode()
    : VariableNodeParser("self", NULL) {}

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

class VariableSuperNode : public VariableNodeParser {
public:
  VariableSuperNode()
    : VariableNodeParser("super", NULL) {}

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
