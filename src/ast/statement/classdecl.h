#ifndef VENOM_AST_CLASSDECL_H
#define VENOM_AST_CLASSDECL_H

#include <string>
#include <vector>

#include <ast/expression/node.h>
#include <ast/statement/node.h>

namespace venom {
namespace ast {

class ClassDeclNode : public ASTStatementNode {
public:
  /** Takes ownership of stmts */
  ClassDeclNode(const std::string&              name,
                const std::vector<std::string>& parents,
                ASTStatementNode*               stmts)
    : name(name), parents(parents), stmts(stmts) {}

  ~ClassDeclNode() {
    delete stmts;
  }
private:
  std::string              name;
  std::vector<std::string> parents;
  ASTStatementNode*        stmts;
};

}
}

#endif /* VENOM_AST_CLASSDECL_H */
