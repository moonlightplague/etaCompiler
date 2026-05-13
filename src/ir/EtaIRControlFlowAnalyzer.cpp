#include "EtaIRControlFlowAnalyzer.hpp"

#include <format>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "ir.hpp"
#include "utils.hpp"

namespace ssy38::my_compiler::visitor {
std::any EtaIRControlFlowAnalyzer::visit(ir::IRNode &node) {
  return node.accept(*this);
}

std::any EtaIRControlFlowAnalyzer::visit(ir::DataDeclaration &node) {
  return ir::DataDeclaration(node);
}

std::any EtaIRControlFlowAnalyzer::visit(ir::FunctionDeclaration &node) {
  auto rewritten_body =
      std::any_cast<std::shared_ptr<ir::Statement>>(node.body->accept(*this));
  return ir::FunctionDeclaration(node.name, rewritten_body, node.arg_count, node.return_val_count);
}

std::any EtaIRControlFlowAnalyzer::visit(ir::CompilationUnit &node) {
  std::vector<ir::FunctionDeclaration> rewritten_functionDeclarations;
  for (auto &functionDeclaration : node.functionDeclarations) {
    auto rewritten_functionDeclaration = std::any_cast<ir::FunctionDeclaration>(
        functionDeclaration.accept(*this));
    rewritten_functionDeclarations.push_back(rewritten_functionDeclaration);
  }
  return ir::CompilationUnit(node.name, node.dataDeclarations,
                             rewritten_functionDeclarations);
}

std::any EtaIRControlFlowAnalyzer::visit(ir::Expression &node) {
  return node.accept(*this);
}

std::any EtaIRControlFlowAnalyzer::visit(ir::IntegerLiteral &node) {
  return std::static_pointer_cast<ir::Expression>(
      std::make_shared<ir::IntegerLiteral>(node));
}

std::any EtaIRControlFlowAnalyzer::visit(ir::Temporary &node) {
  return std::static_pointer_cast<ir::Expression>(
      std::make_shared<ir::Temporary>(node));
}

std::any EtaIRControlFlowAnalyzer::visit(ir::BinaryExpression &node) {
  return std::static_pointer_cast<ir::Expression>(
      std::make_shared<ir::BinaryExpression>(node));
}

std::any EtaIRControlFlowAnalyzer::visit(ir::MemoryAccess &node) {
  return std::static_pointer_cast<ir::Expression>(
      std::make_shared<ir::MemoryAccess>(node));
}

std::any EtaIRControlFlowAnalyzer::visit(ir::FunctionCall &node) {
  return std::static_pointer_cast<ir::Expression>(
      std::make_shared<ir::FunctionCall>(node));
}

std::any EtaIRControlFlowAnalyzer::visit(ir::Address &node) {
  return std::static_pointer_cast<ir::Expression>(
      std::make_shared<ir::Address>(node));
}

std::any EtaIRControlFlowAnalyzer::visit(ir::ExpressionSequence &node) {
  throw std::runtime_error(
      "ExpressionSequence shouldn't appear after IR lowering.");
}

std::any EtaIRControlFlowAnalyzer::visit(ir::Statement &node) {
  return node.accept(*this);
}

std::any EtaIRControlFlowAnalyzer::visit(ir::Assignment &node) {
  return std::static_pointer_cast<ir::Statement>(
      std::make_shared<ir::Assignment>(node));
}

std::any EtaIRControlFlowAnalyzer::visit(ir::StatementSequence &node) {
  std::vector<std::shared_ptr<ir::Statement>> rewritten_statements;
  rewritten_statements.reserve(node.statements.size());

  // first pass: convert all (CJUMP e, ifLabel, elseLabel) to (CJUMP e, ifLabel;
  // JUMP elseLabel)  and unpack all sequences
  for (const auto &statement : node.statements) {
    auto rewritten_stmt =
        std::any_cast<std::shared_ptr<ir::Statement>>(statement->accept(*this));
    auto rewritten_seq =
        std::dynamic_pointer_cast<ir::StatementSequence>(rewritten_stmt);
    if (rewritten_seq != nullptr) {
      rewritten_statements.insert(rewritten_statements.end(),
                                  rewritten_seq->statements.begin(),
                                  rewritten_seq->statements.end());
      continue;
    }
    rewritten_statements.push_back(std::move(rewritten_stmt));
  }

  std::vector<std::shared_ptr<ir::Statement>> optimized_statements;
  optimized_statements.reserve(rewritten_statements.size());

  // second pass: eliminate jumps to labels directly afterwards
  for (size_t i = 0; i < rewritten_statements.size(); ++i) {
    auto jump_stmt =
        std::dynamic_pointer_cast<ir::Jump>(rewritten_statements[i]);
    if (jump_stmt != nullptr && i + 1 < rewritten_statements.size()) {
      auto jump_target =
          std::dynamic_pointer_cast<ir::Address>(jump_stmt->target);
      auto next_label =
          std::dynamic_pointer_cast<ir::Label>(rewritten_statements[i + 1]);
      if (jump_target != nullptr && next_label != nullptr &&
          jump_target->label == next_label->name) {  // jump is eliminated
        continue;
      }
    }
    optimized_statements.push_back(rewritten_statements[i]);
  }

  return std::static_pointer_cast<ir::Statement>(
      std::make_shared<ir::StatementSequence>(std::move(optimized_statements)));
}

std::any EtaIRControlFlowAnalyzer::visit(ir::Jump &node) {
  return std::static_pointer_cast<ir::Statement>(
      std::make_shared<ir::Jump>(node));
}

std::any EtaIRControlFlowAnalyzer::visit(ir::Label &node) {
  return std::static_pointer_cast<ir::Statement>(
      std::make_shared<ir::Label>(node));
}

std::any EtaIRControlFlowAnalyzer::visit(ir::ConditionalJump &node) {
  if(!node.elseLabelName.empty()){
    auto output = std::make_shared<ir::StatementSequence>(
        std::vector<std::shared_ptr<ir::Statement>>{
            std::make_shared<ir::ConditionalJump>(node.condition,
                                                  node.ifLabelName),
            std::make_shared<ir::Jump>(
                std::make_shared<ir::Address>(node.elseLabelName))});
    return std::static_pointer_cast<ir::Statement>(output);
  }
  return std::static_pointer_cast<ir::Statement>(std::make_shared<ir::ConditionalJump>(node));
}

std::any EtaIRControlFlowAnalyzer::visit(ir::CallStatement &node) {
  return std::static_pointer_cast<ir::Statement>(
      std::make_shared<ir::CallStatement>(node));
}

std::any EtaIRControlFlowAnalyzer::visit(ir::ReturnStatement &node) {
  return std::static_pointer_cast<ir::Statement>(
      std::make_shared<ir::ReturnStatement>(node));
}
}  // namespace ssy38::my_compiler::visitor
