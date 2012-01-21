#ifndef VENOM_AST_STATEMENT_NODE_H
#define VENOM_AST_STATEMENT_NODE_H

#include <iostream>
#include <vector>

#include <ast/node.h>
#include <util/stl.h>

namespace venom {

namespace analysis {
  /** Forward decl */
  class InstantiatedType;
}

namespace ast {

class ASTStatementNode : public ASTNode {
  friend class StmtListNode;
public:
  ASTStatementNode() {}
  virtual void typeCheck(analysis::SemanticContext* ctx,
                         analysis::InstantiatedType* expected = NULL);

  VENOM_AST_TYPED_CLONE_STMT(ASTStatementNode)

protected:
  virtual void liftPhaseImpl(analysis::SemanticContext* ctx,
                             analysis::SymbolTable* liftInto,
                             std::vector<ASTStatementNode*>& liftedStmts);

  virtual ASTStatementNode*
    replace(analysis::SemanticContext* ctx, ASTNode* replacement);

  void checkExpectedType(analysis::InstantiatedType* expected);
};

typedef std::vector<ASTStatementNode *> StmtNodeVec;

inline StmtNodeVec * MakeStmtVec1(ASTStatementNode *a0) {
  StmtNodeVec *v = new StmtNodeVec;
  v->push_back(a0);
  return v;
}

inline void PrintStmtNodeVec(std::ostream& o,
                             const StmtNodeVec& stmts,
                             size_t indent) {
  for (StmtNodeVec::const_iterator it = stmts.begin();
       it != stmts.end(); ++it) {
    (*it)->print(o, indent);
    if (it + 1 != stmts.end()) o << std::endl << util::indent(indent);
  }
}

}
}

#endif /* VENOM_AST_STATEMENT_NODE_H */
