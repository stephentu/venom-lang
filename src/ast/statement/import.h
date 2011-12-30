#ifndef VENOM_AST_IMPORT_H
#define VENOM_AST_IMPORT_H

#include <stdexcept>
#include <string>

#include <ast/statement/node.h>
#include <util/stl.h>

namespace venom {
namespace ast {

class ImportStmtNode : public ASTStatementNode {
public:
  ImportStmtNode(const util::StrVec& names)
    : names(names) {}

  std::string getFileName() const;
  std::string getModuleName() const;

  virtual size_t getNumKids() const { return 0; }

  virtual ASTNode* getNthKid(size_t kid) {
    throw std::out_of_range(__func__);
  }

  virtual bool needsNewScope(size_t k) const { return false; }

  virtual void registerSymbol(analysis::SemanticContext* ctx);

  virtual void print(std::ostream& o, size_t indent = 0) {
    o << "(import " << getModuleName() << ")";
  }

private:
  util::StrVec names;
};

}
}

#endif /* VENOM_AST_IMPORT_H */
