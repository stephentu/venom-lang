/**
 * Copyright (c) 2012 Stephen Tu
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names
 * of its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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
