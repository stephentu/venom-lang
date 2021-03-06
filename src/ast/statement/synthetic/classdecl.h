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

#ifndef VENOM_AST_SYNTHETIC_CLASSDECL_H
#define VENOM_AST_SYNTHETIC_CLASSDECL_H

#include <ast/statement/classdecl.h>

namespace venom {

/** forward decl */
namespace analysis { class Type; }

namespace ast {

class ClassDeclNodeSynthetic : public ClassDeclNode {
public:
  ClassDeclNodeSynthetic(
      const std::string& name,
      const std::vector<analysis::InstantiatedType*>& parentTypes,
      const std::vector<analysis::InstantiatedType*>& typeParamTypes,
      ASTStatementNode* stmts,
      analysis::InstantiatedType* instantiation = NULL)
  : ClassDeclNode(name, stmts, instantiation),
    parentTypes(parentTypes), typeParamTypes(typeParamTypes) {
    assert(!parentTypes.empty());
    // TODO: implementation limitation
    assert(parentTypes.size() == 1);
  }

  virtual std::vector<analysis::InstantiatedType*> getParents() const
    { return parentTypes; }

  virtual std::vector<std::string> getTypeParamNames() const;

  virtual std::vector<analysis::InstantiatedType*> getTypeParams() const
    { return typeParamTypes; }

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL_STMT(ClassDeclNode)

  virtual void print(std::ostream& o, size_t indent = 0);

protected:
  virtual void checkAndInitTypeParams(analysis::SemanticContext* ctx);
  virtual void checkAndInitParents(analysis::SemanticContext* ctx);

private:
  std::vector<analysis::InstantiatedType*> parentTypes;
  std::vector<analysis::InstantiatedType*> typeParamTypes;
};

}
}

#endif /* VENOM_AST_SYNTHETIC_CLASSDECL_H */
