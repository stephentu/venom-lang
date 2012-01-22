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

SymbolNode::SymbolNode(
    BaseSymbol* symbol,
    const TypeTranslator& translator,
    InstantiatedType* explicitType)
  : VariableNode(symbol->getName()), explicitType(explicitType) {
  assert(symbol);
  this->symbol     = symbol;
  this->translator = translator;
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
  } else {
    VariableNode::codeGen(cg);
  }
}

void
SymbolNode::print(ostream& o, size_t indent) {
  o << "(symbol-node " << symbol->getFullName();
  if (explicitType) o << " " << explicitType->stringify();
  o << " (sym-addr " << std::hex << symbol << "))";
}

SymbolNode*
SymbolNode::cloneImpl(CloneMode::Type type) {
  assert(type == CloneMode::Semantic);
  return new SymbolNode(symbol, translator, expectedType);
}

ASTExpressionNode*
SymbolNode::cloneForLiftImpl(LiftContext& ctx) {
  // should not clone symbol nodes for lifting...
  VENOM_NOT_REACHED;
}

SymbolNode*
SymbolNode::cloneForTemplateImpl(const TypeTranslator& t) {
  // should not clone symbol nodes for templating...
  VENOM_NOT_REACHED;
}

}
}
