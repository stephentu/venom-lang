#ifndef VENOM_AST_STATEMENT_NODE_H
#define VENOM_AST_STATEMENT_NODE_H

#include <iostream>
#include <vector>

#include <ast/node.h>

#include <util/stl.h>

namespace venom {
namespace ast {

class ASTStatementNode : public ASTNode {
public:
  ASTStatementNode() {}
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
