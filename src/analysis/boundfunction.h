#ifndef VENOM_ANALYSIS_BOUNDFUNCTION_H
#define VENOM_ANALYSIS_BOUNDFUNCTION_H

#include <string>
#include <vector>
#include <utility>

namespace venom {
namespace analysis {

/** Forward decls */
class FuncSymbol;
class InstantiatedType;

namespace {
  typedef std::pair< FuncSymbol*, std::vector< InstantiatedType* > >
          SymTypeParamsPair;
}

class BoundFunction : public SymTypeParamsPair {
public:
  BoundFunction() : SymTypeParamsPair() {}
  BoundFunction(FuncSymbol* symbol,
                const std::vector<InstantiatedType*>& params);
  bool isFullyInstantiated() const;
  bool isCodeGeneratable() const;
  std::string createFuncName() const;
  FuncSymbol* findSpecializedFuncSymbol();
};

}
}

#endif /* VENOM_ANALYSIS_BOUNDFUNCTION_H */
