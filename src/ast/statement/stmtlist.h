#ifndef VENOM_AST_STMTLIST_H
#define VENOM_AST_STMTLIST_H

#include <ast/expression/node.h>
#include <ast/statement/node.h>

#include <util/stl.h>

namespace venom {
namespace ast {

class StmtListNode : public ASTStatementNode {
public:
  StmtListNode(const StmtNodeVec& stmts)
    : stmts(stmts) {}

  ~StmtListNode() {
    util::delete_pointers(stmts.begin(), stmts.end());
  }

private:
  StmtNodeVec stmts;
};

}
}

#endif /* VENOM_AST_STMTLIST_H */
