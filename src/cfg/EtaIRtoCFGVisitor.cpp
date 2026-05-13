#include "EtaIRtoCFGVisitor.hpp"

#include <bit>
#include <cctype>
#include <format>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <variant>

#include "CfgDotWriter.hpp"
#include "EtaIRControlFlowAnalyzer.hpp"
#include "cfg.hpp"
#include "ir.hpp"
#include "opflags.hpp"

// identify converting pass
namespace ssy38::my_compiler::visitor {
std::any EtaIRtoCFGVisitor::visit(ir::IRNode& node) {
  return node.accept(*this);
}

std::any EtaIRtoCFGVisitor::visit(ir::DataDeclaration& node) { return node; }
std::any EtaIRtoCFGVisitor::visit(ir::FunctionDeclaration& node) {
  current_function_name = node.name;
  cfg::ARG_COUNT = node.arg_count;
  cfg::TEMPS.clear();
  for (auto i = 1; i <= node.arg_count; i++) {
    cfg::TEMPS.insert(std::format("_ARG{}", i));
  }
  std::shared_ptr<ir::Statement> body_for_dce = node.body;
  if (std::dynamic_pointer_cast<ir::StatementSequence>(node.body) == nullptr) {
    body_for_dce = std::make_shared<ir::StatementSequence>(
        std::vector<std::shared_ptr<ir::Statement>>{node.body});
  }

  std::any rewritten_body_any = body_for_dce->accept(*this);
  const auto rewritten_body_ptr =
      std::any_cast<std::shared_ptr<ir::Statement>>(&rewritten_body_any);
  if (rewritten_body_ptr == nullptr) {
    throw std::runtime_error(
        "EtaIRtoCFGVisitor expected rewritten function body as ir::Statement");
  }

  auto result = ir::FunctionDeclaration(node.name, *rewritten_body_ptr,
                                        node.arg_count, node.return_val_count);
  return result;
}
std::any EtaIRtoCFGVisitor::visit(ir::CompilationUnit& node) {
  std::vector<ir::FunctionDeclaration> rewritten_functionDeclarations;
  for (auto& functionDeclaration : node.functionDeclarations) {
    auto rewritten_functionDeclaration = std::any_cast<ir::FunctionDeclaration>(
        functionDeclaration.accept(*this));
    rewritten_functionDeclarations.push_back(rewritten_functionDeclaration);
  }
  return ir::CompilationUnit(node.name, node.dataDeclarations,
                             rewritten_functionDeclarations);
}

// Expressions
std::any EtaIRtoCFGVisitor::visit(ir::Expression& node) {
  throw std::runtime_error("Broken EtaIRtoCFGVisitor visitor");
}
std::any EtaIRtoCFGVisitor::visit(ir::IntegerLiteral& node) {
  throw std::runtime_error("Broken EtaIRtoCFGVisitor visitor");
}
std::any EtaIRtoCFGVisitor::visit(ir::Temporary& node) {
  throw std::runtime_error("Broken EtaIRtoCFGVisitor visitor");
}
std::any EtaIRtoCFGVisitor::visit(ir::BinaryExpression& node) {
  throw std::runtime_error("Broken EtaIRtoCFGVisitor visitor");
}
std::any EtaIRtoCFGVisitor::visit(ir::MemoryAccess& node) {
  throw std::runtime_error("Broken EtaIRtoCFGVisitor visitor");
}
std::any EtaIRtoCFGVisitor::visit(ir::FunctionCall& node) {
  throw std::runtime_error("Broken EtaIRtoCFGVisitor visitor");
}
std::any EtaIRtoCFGVisitor::visit(ir::Address& node) {
  throw std::runtime_error("Broken EtaIRtoCFGVisitor visitor");
}
std::any EtaIRtoCFGVisitor::visit(ir::ExpressionSequence& node) {
  throw std::runtime_error("Broken EtaIRtoCFGVisitor visitor");
}

// Statements
std::any EtaIRtoCFGVisitor::visit(ir::Statement& node) {
  return node.accept(*this);
}
std::any EtaIRtoCFGVisitor::visit(ir::Assignment& node) {
  if (std::holds_alternative<ir::Temporary>(node.dest)) {
    auto m = std::get<ir::Temporary>(node.dest);
    cfg::TEMPS.insert(m.name);
    return std::make_shared<cfg::node>(std::make_shared<ir::Assignment>(node),
                                       cfg::stmt_type::Assign);
  }
  return std::make_shared<cfg::node>(std::make_shared<ir::Assignment>(node),
                                     cfg::stmt_type::MemAssign);
}
std::any EtaIRtoCFGVisitor::visit(ir::StatementSequence& node) {
  cfg::COPIES.clear();
  cfg::LABEL2NODE.clear();
  // ir2cfg pass
  std::vector<std::shared_ptr<cfg::node>> node_list;
  node_list.reserve(node.statements.size() + 1);
  node_list.emplace_back(
      std::make_shared<cfg::node>(nullptr, cfg::stmt_type::Start));
  for (auto stmt : node.statements) {
    node_list.emplace_back(
        std::any_cast<std::shared_ptr<cfg::node>>(stmt->accept(*this)));
  }
  cfg::graph g(node_list);
  // node connecting pass
  g.reconnect();
  cfg::write_dot_for_phase(g, optcfg_phases, optcfg_directory, source_filename,
                           current_function_name, "initial");

  // loop until no elimination happens
  std::size_t g_length_previous = std::numeric_limits<std::size_t>::max();
  bool if_reconnect = true;
  while (g_length_previous > g.node_list.size()) {
    g_length_previous = g.node_list.size();
    // unreachable block elimination
    if_reconnect = g.unreachable_elim();
    if (if_reconnect) g.reconnect();

    // copy propagation
    if (copy) {
      if_reconnect = g.copy_propag();
      if (if_reconnect) g.reconnect();
    }

    // dead code elimination
    if (dce) {
      if_reconnect = g.dead_code_elim();
      if (if_reconnect) g.reconnect();
    }

    // conditional constant propagation
    if (cp) {
      g.const_propag();
    }

    // value numbering
    if (vn) {
      g.value_numbering();
    }

    // eliminate extra control-flow nodes
    g.control_flow_elim();
  }
  cfg::write_dot_for_phase(g, optcfg_phases, optcfg_directory, source_filename,
                           current_function_name, "final");

  // cfg2ir pass
  std::vector<std::shared_ptr<ir::Statement>> statements;
  statements.reserve(g.node_list.size() - 1);
  for (auto n : g.node_list) {
    if (n->type != cfg::stmt_type::Start) {
      statements.push_back(n->stmt);
    }
  }
  return std::static_pointer_cast<ir::Statement>(
      std::make_shared<ir::StatementSequence>(statements));
}
std::any EtaIRtoCFGVisitor::visit(ir::Jump& node) {
  return std::make_shared<cfg::node>(std::make_shared<ir::Jump>(node),
                                     cfg::stmt_type::Jump);
}
std::any EtaIRtoCFGVisitor::visit(ir::Label& node) {
  auto n = std::make_shared<cfg::node>(std::make_shared<ir::Label>(node),
                                       cfg::stmt_type::Label);
  return n;
}
std::any EtaIRtoCFGVisitor::visit(ir::ConditionalJump& node) {
  return std::make_shared<cfg::node>(
      std::make_shared<ir::ConditionalJump>(node), cfg::stmt_type::Cjump);
}
std::any EtaIRtoCFGVisitor::visit(ir::CallStatement& node) {
  return std::make_shared<cfg::node>(std::make_shared<ir::CallStatement>(node),
                                     cfg::stmt_type::Call);
}
std::any EtaIRtoCFGVisitor::visit(ir::ReturnStatement& node) {
  return std::make_shared<cfg::node>(
      std::make_shared<ir::ReturnStatement>(node), cfg::stmt_type::Return);
}

}  // namespace ssy38::my_compiler::visitor
