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
  StmtListNode() {}
  StmtListNode(const StmtNodeVec& stmts)
    : stmts(stmts) {}

  ~StmtListNode() {
    util::delete_pointers(stmts.begin(), stmts.end());
  }

  /** Takes ownership of stmt. Returns self for chaining */
  inline StmtListNode* prependStatement(ASTStatementNode* stmt) {
    stmt->setLocationContext(locCtx);
    stmts.insert(stmts.begin(), stmt);
    return this;
  }

  /** Takes ownership of stmt. Returns self for chaining */
  inline StmtListNode* appendStatement(ASTStatementNode* stmt) {
    stmt->setLocationContext(locCtx);
    stmts.push_back(stmt);
    return this;
  }

  virtual size_t getNumKids() const { return stmts.size(); }

  virtual ASTNode* getNthKid(size_t kid) { return stmts.at(kid); }

  virtual void setNthKid(size_t idx, ASTNode* kid) {
    VENOM_CHECK_RANGE(idx, stmts.size());
    VENOM_SAFE_SET_EXPR(stmts[idx], kid);
  }

  virtual bool needsNewScope(size_t k) const {
    VENOM_CHECK_RANGE(k, stmts.size());
    return false;
  }

  virtual void registerSymbol(analysis::SemanticContext* ctx);

  virtual void typeCheck(analysis::SemanticContext* ctx,
                         analysis::InstantiatedType* expected = NULL);

#define IMPL_LOC_CONTEXT(type) \
  virtual void type##LocationContext(uint32_t ctx) { \
    ASTNode::type##LocationContext(ctx); \
    forchild (kid) { \
      if (kid) kid->type##LocationContext(ctx); \
    } endfor \
  }

  IMPL_LOC_CONTEXT(set)
  IMPL_LOC_CONTEXT(add)
  IMPL_LOC_CONTEXT(clear)

#undef IMPL_LOC_CONTEXT

  virtual ASTNode* rewriteLocal(analysis::SemanticContext* ctx,
                                RewriteMode mode);

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL(StmtListNode)

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(stmts" << std::endl << util::indent(indent + 1);
    PrintStmtNodeVec(o, stmts, indent + 1);
    o << ")";
  }

protected:
  virtual ASTNode* rewriteReturn(analysis::SemanticContext* ctx);

private:
  StmtNodeVec stmts;
};

}
}

#endif /* VENOM_AST_STMTLIST_H */
