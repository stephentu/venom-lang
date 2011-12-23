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
  /** Takes ownership of parents and stmts */
  ClassDeclNode(const std::string&   name,
                const TypeStringVec& parents,
                ASTStatementNode*    stmts)
    : name(name), parents(parents), stmts(stmts) {}

  ~ClassDeclNode() {
    util::delete_pointers(parents.begin(), parents.end());
    delete stmts;
  }

  virtual size_t getNumKids() const { return 1; }

  virtual ASTNode* getNthKid(size_t kid) {
    ASTNode *kids[] = {stmts};
    VENOM_SAFE_RETURN(kids, kid);
  }

  virtual bool needsNewScope(size_t k) const { return true; }

  virtual void semanticCheck(analysis::SemanticContext* ctx);

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(class" << std::endl << util::indent(indent + 1);
    stmts->print(o, indent + 1);
    o << ")";
  }

private:
  std::string       name;
  TypeStringVec     parents;
  ASTStatementNode* stmts;
};

}
}

#endif /* VENOM_AST_CLASSDECL_H */
