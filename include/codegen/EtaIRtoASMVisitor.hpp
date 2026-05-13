#pragma once

#include <utility>

#include "EtaIRVisitor.hpp"
#include "asm.hpp"

namespace ssy38::my_compiler::visitor {

const std::vector temps_to_csr = {
  std::make_pair(codegen::TemporaryOperand("_CSRRBX"), codegen::Register::RBX),
  std::make_pair(codegen::TemporaryOperand("_CSRR12"), codegen::Register::R12),
  std::make_pair(codegen::TemporaryOperand("_CSRR13"), codegen::Register::R13),
  std::make_pair(codegen::TemporaryOperand("_CSRR14"), codegen::Register::R14),
  std::make_pair(codegen::TemporaryOperand("_CSRR15"), codegen::Register::R15),
};

/**
 * A visitor that translates IR to abstract assembly.
 */
class EtaIRtoASMVisitor : public EtaIRBaseVisitor {
 public:
  std::any visit(ir::IRNode& node) override;

  std::any visit(ir::DataDeclaration& node) override;
  std::any visit(ir::FunctionDeclaration& node) override;
  std::any visit(ir::CompilationUnit& node) override;

  // Expressions
  std::any visit(ir::Expression& node) override;
  std::any visit(ir::IntegerLiteral& node) override;
  std::any visit(ir::Temporary& node) override;
  std::any visit(ir::BinaryExpression& node) override;
  std::any visit(ir::MemoryAccess& node) override;
  std::any visit(ir::FunctionCall& node) override;
  std::any visit(ir::Address& node) override;
  std::any visit(ir::ExpressionSequence& node) override;

  // Statements
  std::any visit(ir::Statement& node) override;
  std::any visit(ir::Assignment& node) override;
  std::any visit(ir::StatementSequence& node) override;
  std::any visit(ir::Jump& node) override;
  std::any visit(ir::Label& node) override;
  std::any visit(ir::ConditionalJump& node) override;
  std::any visit(ir::CallStatement& node) override;
  std::any visit(ir::ReturnStatement& node) override;
};

/**
 * A helper visitor that assigns an expression to temporary `temp`.
 */
struct EtaASMTemporaryAllocatorVisitor : public EtaIRBaseVisitor {
  codegen::TemporaryOperand temp;

  explicit EtaASMTemporaryAllocatorVisitor(codegen::TemporaryOperand temp)
      : temp(std::move(temp)) {}

  // Expressions
  std::any visit(ir::Expression& node) override;
  std::any visit(ir::IntegerLiteral& node) override;
  std::any visit(ir::Temporary& node) override;
  std::any visit(ir::BinaryExpression& node) override;
  std::any visit(ir::MemoryAccess& node) override;
  std::any visit(ir::Address& node) override;
};
}  // namespace ssy38::my_compiler::visitor