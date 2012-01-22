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

protected:
  TypeMap map;
};

}
}

#endif /* VENOM_ANALYSIS_TYPETRANSLATOR_H */
