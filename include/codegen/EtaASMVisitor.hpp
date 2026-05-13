#pragma once
#include <any>

#include "EtaIRVisitor.hpp"

namespace ssy38::my_compiler::codegen {
struct CommandSequence;
struct ConstantOperand;
struct RegisterOperand;
struct TemporaryOperand;
struct MemoryOperand;
struct LabelOperand;
struct Directive;
struct LabelDeclaration;
struct Operation;
}  // namespace ssy38::my_compiler::codegen

namespace ssy38::my_compiler::visitor {
/**
 * A build visitor abstract class for the Eta ASM tree.
 * A build visitor visits the Eta ASM tree and builds/returns a new object.
 */
class EtaASMVisitor {
 public:
  virtual ~EtaASMVisitor() = default;
  virtual std::any visit(codegen::CommandSequence& node) = 0;
  virtual std::any visit(codegen::ConstantOperand& node) = 0;
  virtual std::any visit(codegen::RegisterOperand& node) = 0;
  virtual std::any visit(codegen::TemporaryOperand& node) = 0;
  virtual std::any visit(codegen::MemoryOperand& node) = 0;
  virtual std::any visit(codegen::LabelOperand& node) = 0;
  virtual std::any visit(codegen::Directive& node) = 0;
  virtual std::any visit(codegen::LabelDeclaration& node) = 0;
  virtual std::any visit(codegen::Operation& node) = 0;
};

/**
 * Default base ASM visitor implementation.
 */
class EtaASMBaseVisitor : public EtaASMVisitor {
 public:
  ~EtaASMBaseVisitor() override = default;

  std::any visit(codegen::CommandSequence& node) override { return nullptr; }
  std::any visit(codegen::ConstantOperand& node) override { return nullptr; }
  std::any visit(codegen::RegisterOperand& node) override { return nullptr; }
  std::any visit(codegen::TemporaryOperand& node) override { return nullptr; }
  std::any visit(codegen::MemoryOperand& node) override { return nullptr; }
  std::any visit(codegen::LabelOperand& node) override { return nullptr; }
  std::any visit(codegen::Directive& node) override { return nullptr; }
  std::any visit(codegen::LabelDeclaration& node) override { return nullptr; }
  std::any visit(codegen::Operation& node) override { return nullptr; }
};
}  // namespace ssy38::my_compiler::visitor
