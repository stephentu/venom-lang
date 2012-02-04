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

#ifndef VENOM_ANALYSIS_TYPETRANSLATOR_H
#define VENOM_ANALYSIS_TYPETRANSLATOR_H

#include <map>
#include <vector>

namespace venom {
namespace analysis {

/** Forward decl */
class BoundFunction;
class InstantiatedType;
class SemanticContext;

class TypeTranslator {
  friend class SymbolTable;
  friend class FuncSymbol;
public:
  typedef std::vector< std::pair<InstantiatedType*, InstantiatedType*> >
          TypeMap;

  TypeTranslator() {}

  InstantiatedType* translate(
      SemanticContext* ctx, InstantiatedType* type) const;

  void translate(
      SemanticContext* ctx,
      BoundFunction& from,
      BoundFunction& result) const;

  void bind(InstantiatedType* obj);

  void bind(BoundFunction& func);

  void addTranslation(InstantiatedType* from, InstantiatedType* to);

  struct TranslateFunctor {
    typedef InstantiatedType* result_type;
    TranslateFunctor(SemanticContext* ctx, const TypeTranslator* t)
      : ctx(ctx), t(t)  {}
    TranslateFunctor(SemanticContext* ctx, const TypeTranslator& t)
      : ctx(ctx), t(&t) {}
    inline InstantiatedType* operator()(InstantiatedType* type) const {
      return t->translate(ctx, type);
    }
  private:
    SemanticContext* ctx;
    const TypeTranslator* t;
  };

  void printStderr() const;

private:
  void bind(const std::vector<InstantiatedType*>& lhs,
            const std::vector<InstantiatedType*>& rhs);

  /**
   * Caller must call with changed = false- this
   * method only sets changed = true when something happens (it does
   * not touch changed if nothing is translated)
   */
  InstantiatedType* translateImpl(
      SemanticContext* ctx, InstantiatedType* type, bool& changed) const;

  struct TranslateImplFunctor {
    typedef InstantiatedType* result_type;
    TranslateImplFunctor(SemanticContext* ctx, const TypeTranslator* t, bool* changed)
      : ctx(ctx), t(t), changed(changed)  {}
    inline InstantiatedType* operator()(InstantiatedType* type) const {
      bool _changed = false;
      InstantiatedType* ret = t->translateImpl(ctx, type, _changed);
      *changed |= _changed;
      return ret;
    }
  private:
    SemanticContext* ctx;
    const TypeTranslator* t;
    bool* changed;
  };

protected:
  TypeMap map;
};

}
}

#endif /* VENOM_ANALYSIS_TYPETRANSLATOR_H */
