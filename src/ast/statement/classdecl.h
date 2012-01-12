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

namespace analysis {
  /** Forward decl */
  class Type;
}

namespace ast {

class ClassDeclNode : public ASTStatementNode {
public:
  /** Takes ownership of parents and stmts */
  ClassDeclNode(const std::string&   name,
                const TypeStringVec& parents,
                const util::StrVec&  typeParams,
                ASTStatementNode*    stmts)
    : name(name), parents(parents),
      typeParams(typeParams), stmts(stmts) {
    stmts->addLocationContext(ASTNode::TopLevelClassBody);
  }

  ~ClassDeclNode() {
    util::delete_pointers(parents.begin(), parents.end());
    delete stmts;
  }

  inline std::string& getName() { return name; }
  inline const std::string& getName() const { return name; }

  inline TypeStringVec& getParents() { return parents; }
  inline const TypeStringVec& getParents() const { return parents; }

  inline util::StrVec& getTypeParams() { return typeParams; }
  inline util::ConstStrVec& getTypeParams() const { return typeParams; }

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

  virtual void registerSymbol(analysis::SemanticContext* ctx);

  virtual void semanticCheckImpl(analysis::SemanticContext* ctx,
                                 bool doRegister);

  virtual analysis::BaseSymbol* getSymbol();

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL(ClassDeclNode)

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(class" << std::endl << util::indent(indent + 1);
    o << "(type-params (" <<
      util::join(typeParams.begin(), typeParams.end(), ",") <<
      "))" << std::endl << util::indent(indent + 1);
    stmts->print(o, indent + 1);
    o << ")";
  }

private:
  std::string        name;
  TypeStringVec      parents;
  util::StrVec       typeParams;
  ASTStatementNode*  stmts;

  std::vector<analysis::Type*> typeParamTypes;
};

}
}

#endif /* VENOM_AST_CLASSDECL_H */
