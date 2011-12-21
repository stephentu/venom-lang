#ifndef VENOM_AST_STATEMENT_NODE_H
#define VENOM_AST_STATEMENT_NODE_H

#include <vector>

#include <ast/node.h>

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

}
}

#endif /* VENOM_AST_STATEMENT_NODE_H */
