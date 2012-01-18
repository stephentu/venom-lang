#include <ast/expression/synthetic/symbolnode.h>

#include <analysis/semanticcontext.h>
#include <analysis/symbol.h>
#include <analysis/symboltable.h>
#include <analysis/type.h>

#include <backend/bytecode.h>
#include <backend/codegenerator.h>

#include <util/macros.h>

using namespace std;
using namespace venom::analysis;
using namespace venom::backend;

namespace venom {
namespace ast {

InstantiatedType*
SymbolNode::typeCheckImpl(SemanticContext* ctx,
                          InstantiatedType* expected,
                          const InstantiatedTypeVec& typeParamArgs) {
  return staticType;
}

void
SymbolNode::codeGen(CodeGenerator& cg) {
  if (ModuleSymbol* msym = dynamic_cast<ModuleSymbol*>(symbol)) {
    // get the class symbol for the module
    ClassSymbol* moduleClassSymbol = msym->getModuleClassSymbol();
    SemanticContext* ctx =
      moduleClassSymbol->getDefinedSymbolTable()->getSemanticContext();

    // find the class reference index for moduleClassSymbol
    bool create;
    size_t classRefIdx =
      cg.enterClass(moduleClassSymbol->getType()->instantiate(ctx), create);
    assert(!create);

    // load the module from the constant pool
    size_t constIdx = cg.createConstant(Constant(classRefIdx), create);

    // push the const onto the stack
    cg.emitInstU32(Instruction::PUSH_CONST, constIdx);
  }
}

void
SymbolNode::print(ostream& o, size_t indent) {
  o << "(synthetic-symbol-node " << symbol->getFullName() << ")";
}

SymbolNode*
SymbolNode::cloneImpl() {
  return new SymbolNode(symbol, staticType, expectedType);
}

SymbolNode*
SymbolNode::cloneForTemplateImpl(const TypeTranslator& t) {
  return new SymbolNode(symbol, staticType, expectedType);
}

}
}
