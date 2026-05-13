#pragma once

#include <any>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "EtaASMVisitor.hpp"

namespace ssy38::my_compiler::codegen {

/**
 * An ASM Node that can be printed in Intel syntax.
 */
struct ASMNode {
  virtual ~ASMNode() = default;

  /**
   * Prints an ASM Node in Intel syntax.
   *
   * @return The printed string of the ASM Node.
   */
  virtual std::string toString() = 0;

  /**
   * Accepts a visitor and performs double dispatch.
   *
   * @param visitor - the visitor to accept
   * @return the created value
   */
  virtual std::any accept(visitor::EtaASMVisitor& visitor) = 0;
};

struct Operand: public ASMNode {};

struct ConstantOperand: public Operand {
  int64_t value;

  ConstantOperand(int64_t value): value(value) {}

  std::string toString() override;

  std::any accept(visitor::EtaASMVisitor& visitor) override {
    return visitor.visit(*this);
  }
};

enum class Register {
  // 64-bit
  RAX, RBX, RCX, RDX, RSI, RDI, RSP, RBP,
  R8,  R9,  R10, R11, R12, R13, R14, R15,
  RIP,
  // 32-bit
  EAX, EBX, ECX, EDX, ESI, EDI, ESP, EBP,
  R8D, R9D, R10D, R11D, R12D, R13D, R14D, R15D,
  // 16-bit
  AX, BX, CX, DX, SI, DI, SP, BP,
  R8W, R9W, R10W, R11W, R12W, R13W, R14W, R15W,
  // 8-bit low
  AL, BL, CL, DL, SIL, DIL, SPL, BPL,
  R8B, R9B, R10B, R11B, R12B, R13B, R14B, R15B,
  // 8-bit high
  AH, BH, CH, DH
};

/**
 * Promotes a register to its 64-bit general-purpose parent.
 *
 * 32-bit, 16-bit, and 8-bit (low or high) views of a GP register are mapped to
 * the enclosing 64-bit register. 64-bit registers and RIP are returned
 * unchanged.
 *
 * @param reg - the register to promote.
 * @return the corresponding 64-bit register.
 */
Register promote_to_64(Register reg);

struct RegisterOperand: public Operand {
  Register reg;

  RegisterOperand(Register reg): reg(reg) {}

  std::string toString() override;

  std::any accept(visitor::EtaASMVisitor& visitor) override {
    return visitor.visit(*this);
  }
};

struct TemporaryOperand: public Operand {
  std::string name;

  TemporaryOperand(std::string name): name(std::move(name)) {}

  bool operator==(const TemporaryOperand&) const = default;

  std::string toString() override;

  std::any accept(visitor::EtaASMVisitor& visitor) override {
    return visitor.visit(*this);
  }
};

/**
 * Creates a fresh temporary operand using the name supplied.
 *
 * @param name name of the temporary.
 * @return the created temporary operand.
 */
TemporaryOperand create_temporary(std::string name);

struct LabelOperand: public Operand {
  std::string name;

  LabelOperand(std::string name): name(std::move(name)) {}

  std::string toString() override;

  std::any accept(visitor::EtaASMVisitor& visitor) override {
    return visitor.visit(*this);
  }
};

enum class Size {
  BYTE_PTR,
  WORD_PTR,
  DWORD_PTR,
  QWORD_PTR
};

struct MemoryOperand: public Operand {
  Size size;
  std::optional<std::variant<ConstantOperand, RegisterOperand, TemporaryOperand,
                             LabelOperand>>
      base;
  std::optional<std::variant<RegisterOperand, TemporaryOperand>> idx;
  std::optional<int> scale;
  std::optional<ConstantOperand> constant;

  MemoryOperand(Size size,
                std::optional<std::variant<ConstantOperand, RegisterOperand, TemporaryOperand, LabelOperand>> base = std::nullopt,
                std::optional<std::variant<RegisterOperand, TemporaryOperand>> idx = std::nullopt,
                std::optional<int> scale = std::nullopt,
                std::optional<ConstantOperand> constant = std::nullopt)
      : size(size), base(std::move(base)), idx(std::move(idx)),
        scale(scale), constant(std::move(constant)) {}

  std::string toString() override;

  std::any accept(visitor::EtaASMVisitor& visitor) override {
    return visitor.visit(*this);
  }
};

struct Command: public ASMNode {};

struct Directive: public Command {
  std::string text;

  Directive(std::string text): text(text){}

  std::string toString() override;

  std::any accept(visitor::EtaASMVisitor& visitor) override {
    return visitor.visit(*this);
  }
};

struct LabelDeclaration: public Command {
  std::string name;

  LabelDeclaration(std::string name): name(name){}
  
  std::string toString() override;

  std::any accept(visitor::EtaASMVisitor& visitor) override {
    return visitor.visit(*this);
  }
};

enum class OpCode {
  MOV,
  ENTER,
  JMP,
  CMP,
  JNE,
  JZ,
  JNZ,
  JL,
  JG,
  JB,
  JGE,
  JLE,
  JS,
  JNS,
  TEST,
  PUSH,
  POP,
  CALL,
  ADD,
  SUB,
  LEAVE,
  RET,
  IMUL,
  AND,
  OR,
  XOR,
  SHL,
  SHR,
  SAR,
  SETE,
  SETNE,
  SETL,
  SETG,
  SETB,
  SETGE,
  SETLE,
  CQO,
  NEG,
  IDIV,
  LEA
};

struct Operation: public Command {
  OpCode opcode;
  std::shared_ptr<Operand> operand1; // nullable
  std::shared_ptr<Operand> operand2; // nullable
  std::optional<int> numTerms; // number of arguments/return values (only used for CALL and RET)

  Operation(OpCode opcode, std::shared_ptr<Operand> operand1, std::shared_ptr<Operand> operand2):
  opcode(opcode), operand1(std::move(operand1)), operand2(std::move(operand2)) {}

  Operation(OpCode opcode, std::shared_ptr<Operand> operand1):
  opcode(opcode), operand1(std::move(operand1)), operand2(nullptr) {}

  Operation(OpCode opcode, std::shared_ptr<Operand> operand1, int numTerms):
  opcode(opcode), operand1(std::move(operand1)), operand2(nullptr), numTerms(numTerms) {}

  Operation(OpCode opcode):
  opcode(opcode), operand1(nullptr), operand2(nullptr) {}

  Operation(OpCode opcode, int numTerms):
  opcode(opcode), numTerms(numTerms) {}

  std::string toString() override;

  std::any accept(visitor::EtaASMVisitor& visitor) override {
    return visitor.visit(*this);
  }
};

struct CommandSequence: public ASMNode {
  std::vector<std::shared_ptr<Command>> commands;
  CommandSequence (std::vector<std::shared_ptr<Command>> commands): commands(std::move(commands)) {}

  std::string toString() override;

  std::any accept(visitor::EtaASMVisitor& visitor) override {
    return visitor.visit(*this);
  }
};

}
