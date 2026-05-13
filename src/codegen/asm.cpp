#include "asm.hpp"

#include <format>
#include <stdexcept>

#include "utils.hpp"

namespace ssy38::my_compiler::codegen {

/**
 * Translates a register to its string representation.
 *
 * @param reg - the register
 * @return the operand name corresponding to a x86-64 register
 */
inline std::string regToString(const Register reg) {
  switch (reg) {
    case Register::RAX:
      return "rax";
    case Register::RBX:
      return "rbx";
    case Register::RCX:
      return "rcx";
    case Register::RDX:
      return "rdx";
    case Register::RSI:
      return "rsi";
    case Register::RDI:
      return "rdi";
    case Register::RSP:
      return "rsp";
    case Register::RBP:
      return "rbp";
    case Register::R8:
      return "r8";
    case Register::R9:
      return "r9";
    case Register::R10:
      return "r10";
    case Register::R11:
      return "r11";
    case Register::R12:
      return "r12";
    case Register::R13:
      return "r13";
    case Register::R14:
      return "r14";
    case Register::R15:
      return "r15";
    case Register::RIP:
      return "rip";
    case Register::EAX:
      return "eax";
    case Register::EBX:
      return "ebx";
    case Register::ECX:
      return "ecx";
    case Register::EDX:
      return "edx";
    case Register::ESI:
      return "esi";
    case Register::EDI:
      return "edi";
    case Register::ESP:
      return "esp";
    case Register::EBP:
      return "ebp";
    case Register::R8D:
      return "r8d";
    case Register::R9D:
      return "r9d";
    case Register::R10D:
      return "r10d";
    case Register::R11D:
      return "r11d";
    case Register::R12D:
      return "r12d";
    case Register::R13D:
      return "r13d";
    case Register::R14D:
      return "r14d";
    case Register::R15D:
      return "r15d";
    case Register::AX:
      return "ax";
    case Register::BX:
      return "bx";
    case Register::CX:
      return "cx";
    case Register::DX:
      return "dx";
    case Register::SI:
      return "si";
    case Register::DI:
      return "di";
    case Register::SP:
      return "sp";
    case Register::BP:
      return "bp";
    case Register::R8W:
      return "r8w";
    case Register::R9W:
      return "r9w";
    case Register::R10W:
      return "r10w";
    case Register::R11W:
      return "r11w";
    case Register::R12W:
      return "r12w";
    case Register::R13W:
      return "r13w";
    case Register::R14W:
      return "r14w";
    case Register::R15W:
      return "r15w";
    case Register::AL:
      return "al";
    case Register::BL:
      return "bl";
    case Register::CL:
      return "cl";
    case Register::DL:
      return "dl";
    case Register::SIL:
      return "sil";
    case Register::DIL:
      return "dil";
    case Register::SPL:
      return "spl";
    case Register::BPL:
      return "bpl";
    case Register::R8B:
      return "r8b";
    case Register::R9B:
      return "r9b";
    case Register::R10B:
      return "r10b";
    case Register::R11B:
      return "r11b";
    case Register::R12B:
      return "r12b";
    case Register::R13B:
      return "r13b";
    case Register::R14B:
      return "r14b";
    case Register::R15B:
      return "r15b";
    case Register::AH:
      return "ah";
    case Register::BH:
      return "bh";
    case Register::CH:
      return "ch";
    case Register::DH:
      return "dh";
  }
  throw std::logic_error("Unknown register");
}

/**
 * Convert a size specifier in an assembly instruction to its Intel syntax
 * string representation.
 *
 * @param size - the size specifier.
 * @return The Intel syntax string corresponding to the size specifier
 */
inline std::string sizeToString(const Size size) {
  switch (size) {
    case Size::BYTE_PTR:
      return "BYTE PTR";
    case Size::WORD_PTR:
      return "WORD PTR";
    case Size::DWORD_PTR:
      return "DWORD PTR";
    case Size::QWORD_PTR:
      return "QWORD PTR";
  }
  throw std::logic_error("Unknown memory operand size");
}

/**
 * Convert the opcode in an assembly instruction to its Intel syntax string
 * representation.
 *
 * @param opcode - the opcode.
 * @return The Intel syntax string corresponding to the size specifier
 */
inline std::string opcodeToString(const OpCode opcode) {
  switch (opcode) {
    case OpCode::MOV:
      return "mov";
    case OpCode::ENTER:
      return "enter";
    case OpCode::JMP:
      return "jmp";
    case OpCode::CMP:
      return "cmp";
    case OpCode::JNE:
      return "jne";
    case OpCode::PUSH:
      return "push";
    case OpCode::POP:
      return "pop";
    case OpCode::CALL:
      return "call";
    case OpCode::ADD:
      return "add";
    case OpCode::SUB:
      return "sub";
    case OpCode::LEAVE:
      return "leave";
    case OpCode::RET:
      return "ret";
    case OpCode::IMUL:
      return "imul";
    case OpCode::AND:
      return "and";
    case OpCode::OR:
      return "or";
    case OpCode::XOR:
      return "xor";
    case OpCode::SHL:
      return "shl";
    case OpCode::SHR:
      return "shr";
    case OpCode::SAR:
      return "sar";
    case OpCode::SETE:
      return "sete";
    case OpCode::SETNE:
      return "setne";
    case OpCode::SETL:
      return "setl";
    case OpCode::SETG:
      return "setg";
    case OpCode::SETB:
      return "setb";
    case OpCode::SETGE:
      return "setge";
    case OpCode::SETLE:
      return "setle";
    case OpCode::CQO:
      return "cqo";
    case OpCode::IDIV:
      return "idiv";
    case OpCode::LEA:
      return "lea";
    case OpCode::NEG:
      return "neg";
    case OpCode::JZ:
      return "jz";
    case OpCode::JNZ:
      return "jnz";
    case OpCode::JL:
      return "jl";
    case OpCode::JG:
      return "jg";
    case OpCode::JB:
      return "jb";
    case OpCode::JGE:
      return "jge";
    case OpCode::JLE:
      return "jle";
    case OpCode::JS:
      return "js";
    case OpCode::JNS:
      return "jns";
    case OpCode::TEST:
      return "test";
  }
  throw std::logic_error("Unknown opcode");
}

Register promote_to_64(const Register reg) {
  switch (reg) {
    case Register::RAX: case Register::EAX: case Register::AX:
    case Register::AL:  case Register::AH:
      return Register::RAX;
    case Register::RBX: case Register::EBX: case Register::BX:
    case Register::BL:  case Register::BH:
      return Register::RBX;
    case Register::RCX: case Register::ECX: case Register::CX:
    case Register::CL:  case Register::CH:
      return Register::RCX;
    case Register::RDX: case Register::EDX: case Register::DX:
    case Register::DL:  case Register::DH:
      return Register::RDX;
    case Register::RSI: case Register::ESI: case Register::SI:
    case Register::SIL:
      return Register::RSI;
    case Register::RDI: case Register::EDI: case Register::DI:
    case Register::DIL:
      return Register::RDI;
    case Register::RSP: case Register::ESP: case Register::SP:
    case Register::SPL:
      return Register::RSP;
    case Register::RBP: case Register::EBP: case Register::BP:
    case Register::BPL:
      return Register::RBP;
    case Register::R8:  case Register::R8D:
    case Register::R8W: case Register::R8B:
      return Register::R8;
    case Register::R9:  case Register::R9D:
    case Register::R9W: case Register::R9B:
      return Register::R9;
    case Register::R10: case Register::R10D:
    case Register::R10W: case Register::R10B:
      return Register::R10;
    case Register::R11: case Register::R11D:
    case Register::R11W: case Register::R11B:
      return Register::R11;
    case Register::R12: case Register::R12D:
    case Register::R12W: case Register::R12B:
      return Register::R12;
    case Register::R13: case Register::R13D:
    case Register::R13W: case Register::R13B:
      return Register::R13;
    case Register::R14: case Register::R14D:
    case Register::R14W: case Register::R14B:
      return Register::R14;
    case Register::R15: case Register::R15D:
    case Register::R15W: case Register::R15B:
      return Register::R15;
    case Register::RIP:
      return Register::RIP;
  }
  throw std::logic_error("Unknown register");
}

/**
 * Creates a FRESH temporary (across both IR and Abstract ASM namespaces) based
 * on a descriptor string.
 *
 * @param name
 * @return the created temporary
 */
TemporaryOperand create_temporary(std::string name) {
  return TemporaryOperand(utils::create_temporary_string(std::move(name)));
}

std::string ConstantOperand::toString() { return std::format("{}", value); }

std::string RegisterOperand::toString() { return regToString(reg); }

std::string TemporaryOperand::toString() { return name; }

std::string MemoryOperand::toString() {
  const auto variantToString = [](auto& operand) { return operand.toString(); };
  std::string result = std::format("{} [", sizeToString(size));
  bool has_term = false;

  if (base.has_value()) {
    result += std::visit(variantToString, base.value());
    has_term = true;
  }

  if (idx.has_value()) {
    const auto idx_string = std::visit(variantToString, idx.value());
    if (scale.has_value()) {
      if (scale.value() >= 0) {
        result += has_term ? std::format(" + {} * {}", idx_string, *scale)
                           : std::format("{} * {}", idx_string, *scale);
      } else {
        result += has_term ? std::format(" - {} * {}", idx_string, -*scale)
                           : std::format("-{} * {}", idx_string, -*scale);
      }
    } else {
      result += has_term ? std::format(" + {}", idx_string)
                         : std::format("{}", idx_string);
    }
    has_term = true;
  }

  if (constant.has_value()) {
    if (constant.value().value >= 0) {
      result += has_term ? std::format(" + {}", constant.value().toString())
                         : constant.value().toString();
    } else {
      result += has_term ? std::format(" - {}", -constant.value().value)
                         : std::format("-{}", -constant.value().value);
    }
  }

  result += "]";
  return result;
}

std::string LabelOperand::toString() { return name; }

std::string Directive::toString() { return text; }

std::string LabelDeclaration::toString() { return std::format("{}:", name); }

std::string Operation::toString() {
  std::string result = opcodeToString(opcode);
  if (operand1) {
    result += std::format(" {}", operand1->toString());
  }
  if (operand2) {
    result += std::format(", {}", operand2->toString());
  }
  return result;
}

std::string CommandSequence::toString() {
  std::string result = "";
  for (const auto& command : commands) {
    result += std::format("{}\n", command->toString());
  }
  return result;
}

}  // namespace ssy38::my_compiler::codegen
