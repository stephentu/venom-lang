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

#ifndef VENOM_AST_SYNTHETIC_FUNCDECL_H
#define VENOM_AST_SYNTHETIC_FUNCDECL_H

#include <ast/statement/funcdecl.h>

namespace venom {
namespace ast {

class FuncDeclNodeSynthetic : public FuncDeclNode {
public:
  FuncDeclNodeSynthetic(
      const std::string& name,
      const std::vector<analysis::InstantiatedType*>& typeParamTypes,
      const ExprNodeVec& params,
      analysis::InstantiatedType* retType,
      ASTStatementNode* stmts)
    : FuncDeclNode(name, params, stmts),
      typeParamTypes(typeParamTypes),
      retType(retType) {
      assert(retType);
    }

  virtual std::vector<std::string> getTypeParamNames() const;

  virtual std::vector<analysis::InstantiatedType*> getTypeParams() const
    { return typeParamTypes; }

  virtual analysis::InstantiatedType* getReturnType() const { return retType; }

  VENOM_AST_TYPED_CLONE_WITH_IMPL_DECL_STMT(FuncDeclNode)

  virtual void print(std::ostream& o, size_t indent = 0);

protected:
  virtual void checkAndInitTypeParams(analysis::SemanticContext* ctx);
  virtual void checkAndInitReturnType(analysis::SemanticContext* ctx);

  std::vector<analysis::InstantiatedType*> typeParamTypes;
  analysis::InstantiatedType* retType;
};

}
}

#endif /* VENOM_AST_SYNTHETIC_FUNCDECL_H */
