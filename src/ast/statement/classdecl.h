#ifndef VENOM_AST_CLASSDECL_H
#define VENOM_AST_CLASSDECL_H

#include <iostream>
#include <string>
#include <vector>

#include <ast/expression/node.h>
#include <ast/statement/node.h>

#include <util/stl.h>
#include <util/macros.h>

namespace venom {
namespace ast {

class ClassDeclNode : public ASTStatementNode {
public:
  ClassDeclNode(const std::string& name,
                ASTStatementNode* stmts)
    : name(name), stmts(stmts) {
    stmts->addLocationContext(ASTNode::TopLevelClassBody);
  }

  ~ClassDeclNode() {
    delete stmts;
  }

  inline std::string& getName() { return name; }
  inline const std::string& getName() const { return name; }

  virtual std::vector<analysis::InstantiatedType*> getParents() const = 0;

  virtual std::vector<analysis::InstantiatedType*> getTypeParams() const = 0;

  virtual size_t getNumKids() const { return 1; }

  virtual ASTNode* getNthKid(size_t kid) {
    ASTNode *kids[] = {stmts};
    VENOM_SAFE_RETURN(kids, kid);
  }

  virtual void setNthKid(size_t idx, ASTNode* kid) {
    VENOM_CHECK_RANGE(idx, 1);
    VENOM_SAFE_SET_EXPR(stmts, kid);
  }

  virtual bool needsNewScope(size_t k) const {
    VENOM_CHECK_RANGE(k, 1);
    return true;
  }

  virtual void semanticCheckImpl(analysis::SemanticContext* ctx,
                                 bool doRegister);

  virtual analysis::BaseSymbol* getSymbol();

protected:
  void registerClassSymbol(
      analysis::SemanticContext* ctx,
      const std::vector<analysis::InstantiatedType*>& parentTypes,
      const std::vector<analysis::InstantiatedType*>& typeParamTypes);

  std::string       name;
  ASTStatementNode* stmts;
};

/** comes from the parser */
class ClassDeclNodeParser : public ClassDeclNode {
public:
  /** Takes ownership of parents */
  ClassDeclNodeParser(const std::string&   name,
                      const TypeStringVec& parents,
                      const util::StrVec&  typeParams,
                      ASTStatementNode*    stmts)
    : ClassDeclNode(name, stmts),
      parents(parents), typeParams(typeParams) {}

  ~ClassDeclNodeParser() {
    util::delete_pointers(parents.begin(), parents.end());
  }

  virtual std::vector<analysis::InstantiatedType*> getParents() const
    { assert(parents.size() == parentTypes.size());
      return parentTypes; }

  virtual std::vector<analysis::InstantiatedType*> getTypeParams() const
    { assert(typeParams.size() == typeParamTypes.size());
      return typeParamTypes; }

  virtual void registerSymbol(analysis::SemanticContext* ctx);

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL(ClassDeclNodeParser)

  virtual void print(std::ostream& o, size_t indent = 0);

private:
  TypeStringVec parents;
  util::StrVec  typeParams;

  std::vector<analysis::InstantiatedType*> parentTypes;
  std::vector<analysis::InstantiatedType*> typeParamTypes;
};

}
}

#endif /* VENOM_AST_CLASSDECL_H */
