#ifndef VENOM_AST_NODE_H
#define VENOM_AST_NODE_H

#include <iostream>

namespace venom {

namespace analysis {
  /** Forward decls **/
  class SemanticContext;
  class SymbolTable;
}

namespace ast {

class ASTNode {
public:
  ASTNode() : symbols(NULL) {}

  virtual ~ASTNode();

  /** Traversal **/

  virtual size_t getNumKids() const      = 0;
  virtual ASTNode* getNthKid(size_t kid) = 0;

  /** Semantic checks **/

  inline analysis::SymbolTable* getSymbolTable() { return symbols; }

  /**
   * Returns true if the k-th child of this AST node represents
   * a new scope, false otherwise.
   */
  virtual bool needsNewScope(size_t k) const = 0;

  /** Call to set up the symbol tables recursively.
   * Must only call once (subsequent calls will trigger assert failures).
   * NOTE: ASTNodes do NOT take ownership of symbols */
  void initSymbolTable(analysis::SymbolTable* symbols);

  /**
   * Perform semantic checks on this node (recursively)
   */
  void semanticCheck(analysis::SemanticContext* ctx) {
    semanticCheckImpl(ctx, true);
  }

  /** Main semanticCheck implementation. Should not be called
   * outside the AST node hierarchy
   * TODO: declare as protected, with appropriate friends
   */
  virtual void semanticCheckImpl(analysis::SemanticContext* ctx,
                                 bool doRegister);

  /**
   * Called when this node should register its symbol in its corresponding
   * symbol table. Note this is not intending to be recursive, so a node should
   * not worry about registering its children. Does nothing by default
   * TODO: declare as protected, with appropriate friends
   */
  virtual void registerSymbol(analysis::SemanticContext* ctx) {}

  /** Debugging helpers **/

  /**
   * Print this AST node, and subsequent children, into ostream.
   * The contract is, when print() is called, the stream is ready
   * for printing. If a node needs to print a newline, it must
   * also print the necessary indents
   */
  virtual void print(std::ostream& o, size_t indent = 0) = 0;

protected:
  analysis::SymbolTable* symbols;
};

}
}

#endif /* VENOM_AST_NODE_H */
