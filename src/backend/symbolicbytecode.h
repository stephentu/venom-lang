#ifndef VENOM_BACKEND_SYMBOLIC_BYTECODE_H
#define VENOM_BACKEND_SYMBOLIC_BYTECODE_H

#include <iostream>

#include <backend/bytecode.h>
#include <backend/codegenerator.h>

namespace venom {
namespace backend {

/** Forward decl */
class Label;

/**
 * A SymbolicInstruction is the type of instruction generated
 * by the CodeGenerator, and is the instruction which is
 * ultimately serialized to a bytecode file.
 *
 * In order to be executable, multiple streams (files) of
 * SymbolicInstructions must be linked together to produce
 * an Instruction stream.
 */
class SymbolicInstruction {
  friend class CodeGenerator;
public:
  typedef Instruction::Opcode Opcode;

  /** Debug helper */
  virtual void printDebug(std::ostream& o) {
    o << Instruction::stringify(opcode) << std::endl;
  }

  virtual ~SymbolicInstruction() {}

protected:
  SymbolicInstruction(Opcode opcode) : opcode(opcode) {}

protected:
  Opcode opcode;
};

template <typename T>
class SInstBase : public SymbolicInstruction {
  friend class CodeGenerator;
protected:
  SInstBase(Opcode opcode, T value) :
    SymbolicInstruction(opcode), value(value) {}
public:
  virtual void printDebug(std::ostream& o) {
    o << Instruction::stringify(opcode) << " " << value << std::endl;
  }
protected:
  T value;
};

class SInstU32 : public SInstBase<uint32_t> {
  friend class CodeGenerator;
protected:
  SInstU32(Opcode opcode, uint32_t value) :
    SInstBase<uint32_t>(opcode, value) {}
};

class SInstI32 : public SInstBase<int32_t> {
  friend class CodeGenerator;
protected:
  SInstI32(Opcode opcode, int32_t value) :
    SInstBase<int32_t>(opcode, value) {}
};

class SInstLabel : public SInstBase<Label*> {
  friend class CodeGenerator;
protected:
  SInstLabel(Opcode opcode, Label* value) :
    SInstBase<Label*>(opcode, value) {}
public:
  virtual void printDebug(std::ostream& o);
};

class SInstI64 : public SInstBase<int64_t> {
  friend class CodeGenerator;
protected:
  SInstI64(Opcode opcode, int64_t value) :
    SInstBase<int64_t>(opcode, value) {}
};

class SInstDouble : public SInstBase<double> {
  friend class CodeGenerator;
protected:
  SInstDouble(Opcode opcode, double value) :
    SInstBase<double>(opcode, value) {}
};

class SInstBool : public SInstBase<bool> {
  friend class CodeGenerator;
protected:
  SInstBool(Opcode opcode, bool value) :
    SInstBase<bool>(opcode, value) {}
};

}
}

#endif /* VENOM_BACKEND_SYMBOLIC_BYTECODE_H */
