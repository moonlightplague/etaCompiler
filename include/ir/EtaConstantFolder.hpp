#pragma once

#include "EtaIRVisitor.hpp"

namespace ssy38::my_compiler::visitor {
/**
 * A visitor that performs constant folding on the IR.
 */
class EtaConstantFolder : public EtaIRBaseVisitor {
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
}  // namespace ssy38::my_compiler::visitor
