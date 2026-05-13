#pragma once

#include <string>
#include <utility>
#include <vector>

#include "EtaIRVisitor.hpp"
#include "cfg.hpp"

namespace ssy38::my_compiler::visitor {
/**
 * A visitor that optimizes IR through dead code elimination.
 */
class EtaIRtoCFGVisitor : public EtaIRBaseVisitor {
 public:
  EtaIRtoCFGVisitor() = default;
  EtaIRtoCFGVisitor(std::vector<std::string> optcfg_phases,
                    std::string optir_directory, std::string source_filename)
      : optcfg_phases(std::move(optcfg_phases)),
        optcfg_directory(std::move(optir_directory)),
        source_filename(std::move(source_filename)) {}

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

 private:
  // record the required cfg visualization phases for this pass
  std::vector<std::string> optcfg_phases;
  // record the output directory for the cfg output files
  std::string optcfg_directory;
  // record the filename of the current eta file
  std::string source_filename;
  // record the current function name which would be used in naming the cfg
  // visualization output file
  std::string current_function_name;
};
}  // namespace ssy38::my_compiler::visitor
