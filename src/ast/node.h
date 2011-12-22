#ifndef VENOM_AST_NODE_H
#define VENOM_AST_NODE_H

#include <iostream>

namespace venom {
namespace ast {

class ASTNode {
public:
  ASTNode() {}
  virtual ~ASTNode() {}

  /** Debugging helpers **/

  /**
   * Print this AST node, and subsequent children, into ostream.
   * The contract is, when print() is called, the stream is ready
   * for printing. If a node needs to print a newline, it must
   * also print the necessary indents
   */
  virtual void print(std::ostream& o, size_t indent = 0) = 0;
};

}
}

#endif /* VENOM_AST_NODE_H */
