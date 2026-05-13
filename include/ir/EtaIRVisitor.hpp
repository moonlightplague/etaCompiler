#pragma once

#include <any>
#include <stdexcept>

namespace ssy38::my_compiler::ir {
struct IRNode;
struct Statement;
struct Expression;
struct DataDeclaration;
struct FunctionDeclaration;
struct CompilationUnit;
struct IntegerLiteral;
struct Temporary;
struct BinaryExpression;
struct MemoryAccess;
struct FunctionCall;
struct Address;
struct ExpressionSequence;
struct Assignment;
struct StatementSequence;
struct Jump;
struct Label;
struct ConditionalJump;
struct CallStatement;
struct ReturnStatement;
}  // namespace ssy38::my_compiler::ir

namespace ssy38::my_compiler::visitor {
/**
 * A build visitor abstract class for the Eta IR tree.
 * A build visitor visits the Eta IR tree and builds/returns a new object.
 */
class EtaIRVisitor {
 public:
  virtual ~EtaIRVisitor() = default;

  virtual std::any visit(ir::IRNode& node) = 0;

  virtual std::any visit(ir::DataDeclaration& node) = 0;
  virtual std::any visit(ir::FunctionDeclaration& node) = 0;
  virtual std::any visit(ir::CompilationUnit& node) = 0;

  // Expressions
  virtual std::any visit(ir::Expression& node) = 0;
  virtual std::any visit(ir::IntegerLiteral& node) = 0;
  virtual std::any visit(ir::Temporary& node) = 0;
  virtual std::any visit(ir::BinaryExpression& node) = 0;
  virtual std::any visit(ir::MemoryAccess& node) = 0;
  virtual std::any visit(ir::FunctionCall& node) = 0;
  virtual std::any visit(ir::Address& node) = 0;
  virtual std::any visit(ir::ExpressionSequence& node) = 0;

  // Statements
  virtual std::any visit(ir::Statement& node) = 0;
  virtual std::any visit(ir::Assignment& node) = 0;
  virtual std::any visit(ir::StatementSequence& node) = 0;
  virtual std::any visit(ir::Jump& node) = 0;
  virtual std::any visit(ir::Label& node) = 0;
  virtual std::any visit(ir::ConditionalJump& node) = 0;
  virtual std::any visit(ir::CallStatement& node) = 0;
  virtual std::any visit(ir::ReturnStatement& node) = 0;
};

/**
 * Default base IR visitor implementation.
 */
class EtaIRBaseVisitor : public EtaIRVisitor {
 public:
  ~EtaIRBaseVisitor() override = default;

  std::any visit(ir::IRNode& node) override { return nullptr; }

  std::any visit(ir::DataDeclaration& node) override { return nullptr; }
  std::any visit(ir::FunctionDeclaration& node) override { return nullptr; }
  std::any visit(ir::CompilationUnit& node) override { return nullptr; }

  // Expressions
  std::any visit(ir::Expression& node) override { return nullptr; }
  std::any visit(ir::IntegerLiteral& node) override { return nullptr; }
  std::any visit(ir::Temporary& node) override { return nullptr; }
  std::any visit(ir::BinaryExpression& node) override { return nullptr; }
  std::any visit(ir::MemoryAccess& node) override { return nullptr; }
  std::any visit(ir::FunctionCall& node) override { return nullptr; }
  std::any visit(ir::Address& node) override { return nullptr; }
  std::any visit(ir::ExpressionSequence& node) override { return nullptr; }

  // Statements
  std::any visit(ir::Statement& node) override { return nullptr; }
  std::any visit(ir::Assignment& node) override { return nullptr; }
  std::any visit(ir::StatementSequence& node) override { return nullptr; }
  std::any visit(ir::Jump& node) override { return nullptr; }
  std::any visit(ir::Label& node) override { return nullptr; }
  std::any visit(ir::ConditionalJump& node) override { return nullptr; }
  std::any visit(ir::CallStatement& node) override { return nullptr; }
  std::any visit(ir::ReturnStatement& node) override { return nullptr; }
};

}  // namespace ssy38::my_compiler::visitor
