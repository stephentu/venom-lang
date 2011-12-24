#ifndef VENOM_AST_STMTLIST_H
#define VENOM_AST_STMTLIST_H

#include <ast/expression/node.h>
#include <ast/statement/node.h>

#include <util/stl.h>
#include <util/macros.h>

namespace venom {
namespace ast {

class StmtListNode : public ASTStatementNode {
public:
  StmtListNode(const StmtNodeVec& stmts)
    : stmts(stmts) {}

  ~StmtListNode() {
    util::delete_pointers(stmts.begin(), stmts.end());
  }

  virtual size_t getNumKids() const { return stmts.size(); }

  virtual ASTNode* getNthKid(size_t kid) { return stmts.at(kid); }

  virtual bool needsNewScope(size_t k) const { return false; }

  virtual void setLocationContext(LocationCtx ctx) {
    forchild (kid) {
      if (kid) kid->setLocationContext(ctx);
    } endfor
  }

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(stmts" << std::endl << util::indent(indent + 1);
    PrintStmtNodeVec(o, stmts, indent + 1);
    o << ")";
  }

private:
  StmtNodeVec stmts;
};

}
}

#endif /* VENOM_AST_STMTLIST_H */
