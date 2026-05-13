#include "EtaMIRtoLIRVisitor.hpp"

#include <format>
#include <memory>
#include <utility>
#include <vector>

#include "ir.hpp"

namespace ssy38::my_compiler::visitor {
using LoweredExpr = std::pair<std::vector<std::shared_ptr<ir::Statement>>,
                              std::shared_ptr<ir::Expression>>;
using LoweredStmt = std::vector<std::shared_ptr<ir::Statement>>;

std::any EtaMIRtoLIRVisitor::visit(ir::IRNode& node) {
  return node.accept(*this);
}

std::any EtaMIRtoLIRVisitor::visit(ir::DataDeclaration& node) {
  // no need to lower a DATA node
  return node;
}

std::any EtaMIRtoLIRVisitor::visit(ir::FunctionDeclaration& node) {
  auto lowered_stmts = std::any_cast<LoweredStmt>(node.body->accept(*this));
  auto output = ir::FunctionDeclaration(
      node.name, std::make_shared<ir::StatementSequence>(lowered_stmts), node.arg_count, node.return_val_count);
  return output;
}

std::any EtaMIRtoLIRVisitor::visit(ir::CompilationUnit& node) {
  auto lowered_decls = std::vector<ir::DataDeclaration>{};
  auto lowered_funcs = std::vector<ir::FunctionDeclaration>{};
  // lower all the declarations (no effect)
  for (auto& decl : node.dataDeclarations) {
    lowered_decls.push_back(
        std::any_cast<ir::DataDeclaration>(decl.accept(*this)));
  }
  // lower all the functions
  for (auto& func : node.functionDeclarations) {
    lowered_funcs.push_back(
        std::any_cast<ir::FunctionDeclaration>(func.accept(*this)));
  }
  auto output = ir::CompilationUnit(node.name, lowered_decls, lowered_funcs);
  return output;
}

// Expressions
std::any EtaMIRtoLIRVisitor::visit(ir::Expression& node) {
  return node.accept(*this);
}

std::any EtaMIRtoLIRVisitor::visit(ir::IntegerLiteral& node) {
  // integer literals have no side effects
  auto stmt = std::vector<std::shared_ptr<ir::Statement>>{};
  auto expr = static_cast<std::shared_ptr<ir::Expression>>(
      std::make_shared<ir::IntegerLiteral>(node));
  return LoweredExpr(std::move(stmt), std::move(expr));
}

std::any EtaMIRtoLIRVisitor::visit(ir::Temporary& node) {
  // temporaries have no side effects
  auto stmt = std::vector<std::shared_ptr<ir::Statement>>{};
  auto expr = static_cast<std::shared_ptr<ir::Expression>>(
      std::make_shared<ir::Temporary>(node));
  return LoweredExpr(std::move(stmt), std::move(expr));
}

std::any EtaMIRtoLIRVisitor::visit(ir::BinaryExpression& node) {
  // we assume e1 and e2 doesn't commute for now
  auto [s1, e1_prime] = std::any_cast<LoweredExpr>(node.left->accept(*this));
  auto [s2, e2_prime] = std::any_cast<LoweredExpr>(node.right->accept(*this));
  if (s2.empty()) { // e2 does not have side effect, can use optimized version
    return LoweredExpr(std::move(s1), std::make_shared<ir::BinaryExpression>(e1_prime, e2_prime, node.op));
  } else { // e2 (i.e. s2) potentially has side effects, play it safe
    auto stmt_list = s1;
    auto t1 = ir::create_temporary("t1");
    stmt_list.push_back(std::make_shared<ir::Assignment>(  // t_1 <- e_1'
        ir::Destination(ir::Temporary(t1)), e1_prime));
    stmt_list.insert(stmt_list.end(), s2.begin(), s2.end());
    auto final_expr = std::make_shared<ir::BinaryExpression>(  // t1 OP e_2'
        std::make_shared<ir::Temporary>(t1), e2_prime, node.op);
    return LoweredExpr(std::move(stmt_list), std::move(final_expr));
  }
}

std::any EtaMIRtoLIRVisitor::visit(ir::MemoryAccess& node) {
  auto [stmt, address_expr] =
      std::any_cast<LoweredExpr>(node.expr->accept(*this));
  auto expr = static_cast<std::shared_ptr<ir::Expression>>(
      std::make_shared<ir::MemoryAccess>(address_expr));
  return LoweredExpr(std::move(stmt), std::move(expr));
}

std::any EtaMIRtoLIRVisitor::visit(ir::FunctionCall& node) {
  int count = 0;
  auto stmt_list = std::vector<std::shared_ptr<ir::Statement>>{};
  auto call_arguments_list = std::vector<std::shared_ptr<ir::Expression>>{};
  auto rv1 = ir::Temporary("_RV1");
  for (auto argument : node.arguments) {
    auto t = ir::create_temporary(std::format("t{}", count));
    auto [stmt, address_expr] =
        std::any_cast<LoweredExpr>(argument->accept(*this));
    stmt_list.insert(stmt_list.end(), stmt.begin(), stmt.end());
    stmt_list.push_back(std::make_shared<ir::Assignment>(
        ir::Destination(ir::Temporary(t)), address_expr));
    call_arguments_list.push_back(std::make_shared<ir::Temporary>(t));
    count++;
  }
  stmt_list.push_back(std::make_shared<ir::CallStatement>(
      node.function, call_arguments_list, 1));
  auto final_expr = std::make_shared<ir::Temporary>(rv1);
  return LoweredExpr(std::move(stmt_list), std::move(final_expr));
}

// we only use side-effect-free expressions inside address nodes
std::any EtaMIRtoLIRVisitor::visit(ir::Address& node) {
  auto stmt = std::vector<std::shared_ptr<ir::Statement>>{};
  auto expr = static_cast<std::shared_ptr<ir::Expression>>(
      std::make_shared<ir::Address>(node));
  return LoweredExpr(std::move(stmt), std::move(expr));
}

std::any EtaMIRtoLIRVisitor::visit(ir::ExpressionSequence& node) {
  auto lowered_s = std::any_cast<LoweredStmt>(node.statement->accept(*this));
  auto [stmt, address_expr] =
      std::any_cast<LoweredExpr>(node.expression->accept(*this));
  lowered_s.insert(lowered_s.end(), stmt.begin(), stmt.end());
  return LoweredExpr(std::move(lowered_s), std::move(address_expr));
}

// Statements
std::any EtaMIRtoLIRVisitor::visit(ir::Statement& node) {
  return node.accept(*this);
}
std::any EtaMIRtoLIRVisitor::visit(ir::Assignment& node) {
  auto stmt_list = std::vector<std::shared_ptr<ir::Statement>>{};
  auto [rhs_statements, rhs_expr] =
      std::any_cast<LoweredExpr>(node.expression->accept(*this));

  if (const auto* dest_temp = std::get_if<ir::Temporary>(&node.dest)) {
    stmt_list.insert(stmt_list.end(), rhs_statements.begin(),
                     rhs_statements.end());
    stmt_list.push_back(std::make_shared<ir::Assignment>(
        ir::Destination(*dest_temp), rhs_expr));
    return LoweredStmt(std::move(stmt_list));
  }

  if (const auto* dest_mem = std::get_if<ir::MemoryAccess>(&node.dest)) {
    auto [addr_statements, addr_expr] =
      std::any_cast<LoweredExpr>(dest_mem->expr->accept(*this));
    stmt_list.insert(stmt_list.end(), addr_statements.begin(),
                     addr_statements.end());

    if (rhs_statements.empty()) { // rhs has no side effects, no need to create temporary
      stmt_list.push_back(std::make_shared<ir::Assignment>(
        ir::Destination(ir::MemoryAccess(addr_expr)), rhs_expr)
      );
      return LoweredStmt(std::move(stmt_list));
    } else { // rhs potentially has side effects, create temporary for address expression
      stmt_list.insert(stmt_list.end(), rhs_statements.begin(),
                       rhs_statements.end());

      auto t = ir::create_temporary("t");
      stmt_list.push_back(std::make_shared<ir::Assignment>(
          ir::Destination(ir::Temporary(t)), addr_expr));
      stmt_list.push_back(std::make_shared<ir::Assignment>(
          ir::Destination(ir::MemoryAccess(std::make_shared<ir::Temporary>(t))),
          rhs_expr));
      return LoweredStmt(std::move(stmt_list));
    }
  }

  throw std::runtime_error("unexpected destination type");
}

std::any EtaMIRtoLIRVisitor::visit(ir::StatementSequence& node) {
  auto stmt_list = std::vector<std::shared_ptr<ir::Statement>>{};
  for (const auto& statement : node.statements) {
    auto lowered_stmts = std::any_cast<LoweredStmt>(statement->accept(*this));
    stmt_list.insert(stmt_list.end(), lowered_stmts.begin(),
                     lowered_stmts.end());
  }
  return LoweredStmt(std::move(stmt_list));
}

std::any EtaMIRtoLIRVisitor::visit(ir::Jump& node) {
  auto [stmt, target_expr] =
      std::any_cast<LoweredExpr>(node.target->accept(*this));
  stmt.push_back(std::make_shared<ir::Jump>(target_expr));
  return LoweredStmt(std::move(stmt));
}

std::any EtaMIRtoLIRVisitor::visit(ir::Label& node) {
  return LoweredStmt({std::make_shared<ir::Label>(node)});
}

std::any EtaMIRtoLIRVisitor::visit(ir::ConditionalJump& node) {
  auto [stmt, target_expr] =
      std::any_cast<LoweredExpr>(node.condition->accept(*this));
  stmt.push_back(std::make_shared<ir::ConditionalJump>(
      target_expr, node.ifLabelName, node.elseLabelName));
  return LoweredStmt(std::move(stmt));
}

// for call, we lower each of the arguments and then hoist the produced
// statement for argument and store the exprs in temporaries
std::any EtaMIRtoLIRVisitor::visit(ir::CallStatement& node) {
  int count = 0;
  auto stmt_list = std::vector<std::shared_ptr<ir::Statement>>{};
  auto call_arguments_list = std::vector<std::shared_ptr<ir::Expression>>{};
  for (const auto argument : node.arguments) {
    auto t = ir::create_temporary(std::format("t{}", count));
    auto [stmt, address_expr] =
        std::any_cast<LoweredExpr>(argument->accept(*this));
    stmt_list.insert(stmt_list.end(), stmt.begin(), stmt.end());
    stmt_list.push_back(std::make_shared<ir::Assignment>(
        ir::Destination(ir::Temporary(t)), address_expr));
    call_arguments_list.push_back(std::make_shared<ir::Temporary>(t));
    count++;
  }
  stmt_list.push_back(std::make_shared<ir::CallStatement>(
      node.function, call_arguments_list, node.numReturns));
  return LoweredStmt(std::move(stmt_list));
}
std::any EtaMIRtoLIRVisitor::visit(ir::ReturnStatement& node) {
  int count = 0;
  auto stmt_list = std::vector<std::shared_ptr<ir::Statement>>{};
  auto call_arguments_list = std::vector<std::shared_ptr<ir::Expression>>{};
  for (const auto argument : node.results) {
    auto t = ir::create_temporary(std::format("t{}", count));
    auto [stmt, address_expr] =
        std::any_cast<LoweredExpr>(argument->accept(*this));
    stmt_list.insert(stmt_list.end(), stmt.begin(), stmt.end());
    stmt_list.push_back(std::make_shared<ir::Assignment>(
        ir::Destination(ir::Temporary(t)), address_expr));
    call_arguments_list.push_back(std::make_shared<ir::Temporary>(t));
    count++;
  }
  stmt_list.push_back(
      std::make_shared<ir::ReturnStatement>(call_arguments_list));
  return LoweredStmt(std::move(stmt_list));
};
}  // namespace ssy38::my_compiler::visitor
