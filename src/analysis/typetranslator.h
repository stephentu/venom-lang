#ifndef VENOM_ANALYSIS_TYPETRANSLATOR_H
#define VENOM_ANALYSIS_TYPETRANSLATOR_H

#include <map>
#include <vector>

namespace venom {
namespace analysis {

/** Forward decl */
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

  void bind(InstantiatedType* obj);

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
