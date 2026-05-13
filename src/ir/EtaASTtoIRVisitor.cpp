#pragma clang diagnostic push
#pragma ide diagnostic ignored "ArgumentSelectionDefects"
//
// Created by Apple on 3/5/26.
//

#include "EtaASTtoIRVisitor.hpp"

#include <format>
#include <vector>

#include "ast.hpp"
#include "utils.hpp"

namespace ssy38::my_compiler::visitor {
std::any EtaASTtoIRVisitor::visit(const ast::ASTNode& ast) const {
  return ast.accept(*this);
};

std::any EtaASTtoIRVisitor::visit(const ast::Use& use)const { return nullptr; };

// Expressions
std::any EtaASTtoIRVisitor::visit(const ast::Expression& expr)const  {
  return expr.accept(*this);
}
std::any EtaASTtoIRVisitor::visit(const ast::UnaryExpression& expr)const {
  const auto e =
      std::any_cast<std::shared_ptr<ir::Expression>>(expr.expr->accept(*this));
  switch (expr.op) {
    case ast::Expression::UnaryOperand::Minus: {
      const auto result_ptr = std::make_shared<ir::BinaryExpression>(
          std::make_shared<ir::IntegerLiteral>(0), e, ir::BinaryOperator::Sub);
      return static_cast<std::shared_ptr<ir::Expression>>(result_ptr);
    }
    case ast::Expression::UnaryOperand::Bang: {
      const auto result_ptr = std::make_shared<ir::BinaryExpression>(
          e, std::make_shared<ir::IntegerLiteral>(1), ir::BinaryOperator::Xor);
      return static_cast<std::shared_ptr<ir::Expression>>(result_ptr);
    }
  }
  throw std::runtime_error("unexpected unary operator");
}
std::any EtaASTtoIRVisitor::visit(const ast::BinaryExpression& expr)const {
  const auto e1 =
      std::any_cast<std::shared_ptr<ir::Expression>>(expr.left->accept(*this));
  const auto e2 =
      std::any_cast<std::shared_ptr<ir::Expression>>(expr.right->accept(*this));
  switch (expr.op) {
    case ast::Expression::BinaryOperand::Multiply: {
      const auto result_ptr = std::make_shared<ir::BinaryExpression>(
          e1, e2, ir::BinaryOperator::Mul);
      return static_cast<std::shared_ptr<ir::Expression>>(result_ptr);
    }
    case ast::Expression::BinaryOperand::HighMultiply: {
      const auto result_ptr = std::make_shared<ir::BinaryExpression>(
          e1, e2, ir::BinaryOperator::Hmul);
      return static_cast<std::shared_ptr<ir::Expression>>(result_ptr);
    }
    case ast::Expression::BinaryOperand::Divide: {
      const auto result_ptr = std::make_shared<ir::BinaryExpression>(
          e1, e2, ir::BinaryOperator::Div);
      return static_cast<std::shared_ptr<ir::Expression>>(result_ptr);
    }
    case ast::Expression::BinaryOperand::Mod: {
      const auto result_ptr = std::make_shared<ir::BinaryExpression>(
          e1, e2, ir::BinaryOperator::Mod);
      return static_cast<std::shared_ptr<ir::Expression>>(result_ptr);
    }
    case ast::Expression::BinaryOperand::Minus: {
      const auto result_ptr = std::make_shared<ir::BinaryExpression>(
          e1, e2, ir::BinaryOperator::Sub);
      return static_cast<std::shared_ptr<ir::Expression>>(result_ptr);
    }
    case ast::Expression::BinaryOperand::LessThan: {
      const auto result_ptr = std::make_shared<ir::BinaryExpression>(
          e1, e2, ir::BinaryOperator::Lt);
      return static_cast<std::shared_ptr<ir::Expression>>(result_ptr);
    }
    case ast::Expression::BinaryOperand::LessThanEqual: {
      const auto result_ptr = std::make_shared<ir::BinaryExpression>(
          e1, e2, ir::BinaryOperator::Leq);
      return static_cast<std::shared_ptr<ir::Expression>>(result_ptr);
    }
    case ast::Expression::BinaryOperand::GreaterThan: {
      const auto result_ptr = std::make_shared<ir::BinaryExpression>(
          e1, e2, ir::BinaryOperator::Gt);
      return static_cast<std::shared_ptr<ir::Expression>>(result_ptr);
    }
    case ast::Expression::BinaryOperand::GreaterThanEqual: {
      const auto result_ptr = std::make_shared<ir::BinaryExpression>(
          e1, e2, ir::BinaryOperator::Geq);
      return static_cast<std::shared_ptr<ir::Expression>>(result_ptr);
    }
    case ast::Expression::BinaryOperand::Equal: {
      const auto result_ptr = std::make_shared<ir::BinaryExpression>(
          e1, e2, ir::BinaryOperator::Eq);
      return static_cast<std::shared_ptr<ir::Expression>>(result_ptr);
    }
    case ast::Expression::BinaryOperand::NotEqual: {
      const auto result_ptr = std::make_shared<ir::BinaryExpression>(
          e1, e2, ir::BinaryOperator::Neq);
      return static_cast<std::shared_ptr<ir::Expression>>(result_ptr);
    }
    case ast::Expression::BinaryOperand::And: {
      auto t = ir::create_temporary("t");
      auto l1 = ir::create_label_name("l1");
      auto l2 = ir::create_label_name("l2");
      auto lf = ir::create_label_name("lf");

      std::vector<std::shared_ptr<ir::Statement>> statements = {
          std::make_shared<ir::Assignment>(  // t <- 0
              ir::Destination(t), std::make_shared<ir::IntegerLiteral>(0)),
          std::make_shared<ir::ConditionalJump>(  // cjump e1, l1, lf
              e1, l1, lf),
          std::make_shared<ir::Label>(l1),
          std::make_shared<ir::ConditionalJump>(  // cjump e2, l2, lf
              e2, l2, lf),
          std::make_shared<ir::Label>(l2),
          std::make_shared<ir::Assignment>(  // t <- 1
              ir::Destination(t), std::make_shared<ir::IntegerLiteral>(1)),
          std::make_shared<ir::Label>(lf)};

      const auto result_ptr = std::make_shared<ir::ExpressionSequence>(
          std::make_shared<ir::StatementSequence>(statements),
          std::make_shared<ir::Temporary>(t));
      return static_cast<std::shared_ptr<ir::Expression>>(result_ptr);
    }
    case ast::Expression::BinaryOperand::Or: {
      auto t = ir::create_temporary("t");
      auto l1 = ir::create_label_name("l1");
      auto l2 = ir::create_label_name("l2");
      auto lt = ir::create_label_name("lt");

      std::vector<std::shared_ptr<ir::Statement>> statements = {
          std::make_shared<ir::Assignment>(  // t <- 1
              ir::Destination(t), std::make_shared<ir::IntegerLiteral>(1)),
          std::make_shared<ir::ConditionalJump>(  // cjump e1, lt, l1
              e1, lt, l1),
          std::make_shared<ir::Label>(l1),
          std::make_shared<ir::ConditionalJump>(  // cjump e2, lt, l2
              e2, lt, l2),
          std::make_shared<ir::Label>(l2),
          std::make_shared<ir::Assignment>(  // t <- 0
              ir::Destination(t), std::make_shared<ir::IntegerLiteral>(0)),
          std::make_shared<ir::Label>(lt)};

      const auto result_ptr = std::make_shared<ir::ExpressionSequence>(
          std::make_shared<ir::StatementSequence>(statements),
          std::make_shared<ir::Temporary>(t));
      return static_cast<std::shared_ptr<ir::Expression>>(result_ptr);
    }
    case ast::Expression::BinaryOperand::ArrayAccess: {
      auto t_a = ir::create_temporary("t_a");
      auto t_i = ir::create_temporary("t_i");
      auto l_err = ir::create_label_name("l_err");
      auto l_ok = ir::create_label_name("l_ok");
      std::vector<std::shared_ptr<ir::Statement>> statements = {
          std::make_shared<ir::Assignment>(  // t_a <- e1
              ir::Destination(t_a), e1),
          std::make_shared<ir::Assignment>(  // t_i <- e2
              ir::Destination(t_i), e2),
          std::make_shared<ir::ConditionalJump>(  // cjump (t_i ult [t_a add -8])
                                                  // l_ok l_err
              std::make_shared<ir::BinaryExpression>(  // t_i ult [t_a add -8]
                  std::make_shared<ir::Temporary>(t_i),
                  std::make_shared<ir::MemoryAccess>(  // [t_a add -8]
                      std::make_shared<ir::BinaryExpression>(
                          std::make_shared<ir::Temporary>(t_a),
                          std::make_shared<ir::IntegerLiteral>(-8),
                          ir::BinaryOperator::Add)),
                  ir::BinaryOperator::Ult),
              l_ok, l_err),
          std::make_shared<ir::Label>(l_err),
          std::make_shared<ir::CallStatement>(
              std::make_shared<ir::Address>("_eta_out_of_bounds"),
              std::vector<std::shared_ptr<ir::Expression>>{}, 0),
          std::make_shared<ir::Label>(l_ok)};
      const auto result_ptr = std::make_shared<ir::ExpressionSequence>(
          std::make_shared<ir::StatementSequence>(statements),
          std::make_shared<ir::MemoryAccess>(  // [t_a add (t_i mul 8)]
              std::make_shared<ir::BinaryExpression>(
                  std::make_shared<ir::Temporary>(t_a),
                  std::make_shared<ir::BinaryExpression>(
                      std::make_shared<ir::Temporary>(t_i),
                      std::make_shared<ir::IntegerLiteral>(8),
                      ir::BinaryOperator::Mul),
                  ir::BinaryOperator::Add)));
      return static_cast<std::shared_ptr<ir::Expression>>(result_ptr);
    }

    case ast::Expression::BinaryOperand::Plus: {
      const auto* left_int =
          dynamic_cast<types::IntType*>(expr.left->judgement.type.get());
      const auto* right_int =
          dynamic_cast<types::IntType*>(expr.right->judgement.type.get());
      const auto* left_array =
          dynamic_cast<types::ArrayType*>(expr.left->judgement.type.get());
      const auto* right_array =
          dynamic_cast<types::ArrayType*>(expr.right->judgement.type.get());

      if (left_int && right_int) {
        const auto result_ptr = std::make_shared<ir::BinaryExpression>(
            e1, e2, ir::BinaryOperator::Add);
        return static_cast<std::shared_ptr<ir::Expression>>(result_ptr);
      }
      if (left_array && right_array) {
        auto t = ir::create_temporary("t");
        auto t_1 = ir::create_temporary("t_1");
        auto t_2 = ir::create_temporary("t_2");
        auto t_1n = ir::create_temporary("t_1n");
        auto t_2n = ir::create_temporary("t_2n");
        auto t_n = ir::create_temporary("t_n");
        auto l_0 = ir::create_label_name("l_0");
        auto l_1 = ir::create_label_name("l_1");
        auto l_1p = ir::create_label_name("l_1p");
        auto l_2 = ir::create_label_name("l_2");
        auto l_2p = ir::create_label_name("l_2p");
        std::vector<std::shared_ptr<ir::Statement>> statements = {
            std::make_shared<ir::Assignment>(  // t_1 <- e1
                ir::Destination(t_1), e1),
            std::make_shared<ir::Assignment>(  // t_2 <- e2
                ir::Destination(t_2), e2),
            std::make_shared<ir::Assignment>(  // t_1n <- [t_1 add -8]
                ir::Destination(t_1n),
                std::make_shared<ir::MemoryAccess>(
                    std::make_shared<ir::BinaryExpression>(
                        std::make_shared<ir::Temporary>(t_1),
                        std::make_shared<ir::IntegerLiteral>(-8),
                        ir::BinaryOperator::Add))),
            std::make_shared<ir::Assignment>(  // t_2n <- [t_2 add -8]
                ir::Destination(t_2n),
                std::make_shared<ir::MemoryAccess>(
                    std::make_shared<ir::BinaryExpression>(
                        std::make_shared<ir::Temporary>(t_2),
                        std::make_shared<ir::IntegerLiteral>(-8),
                        ir::BinaryOperator::Add))),
            std::make_shared<ir::Assignment>(  // t_n <- t_1n add t_2n
                ir::Destination(t_n), std::make_shared<ir::BinaryExpression>(
                                          std::make_shared<ir::Temporary>(t_1n),
                                          std::make_shared<ir::Temporary>(t_2n),
                                          ir::BinaryOperator::Add)),
            std::make_shared<ir::Assignment>(  // t <- call (&_eta_alloc) ((t_n
                                               // mul 8) add 8)
                ir::Destination(t),
                std::make_shared<ir::FunctionCall>(
                    std::make_shared<ir::Address>("_eta_alloc"),
                    std::vector<std::shared_ptr<ir::Expression>>{
                        std::make_shared<ir::BinaryExpression>(  // (t_n mul 8)
                                                                 // add 8
                            std::make_shared<ir::BinaryExpression>(
                                std::make_shared<ir::Temporary>(t_n),
                                std::make_shared<ir::IntegerLiteral>(8),
                                ir::BinaryOperator::Mul),
                            std::make_shared<ir::IntegerLiteral>(8),
                            ir::BinaryOperator::Add)})),
            std::make_shared<ir::Assignment>(  // [t] <- t_n
                ir::Destination(
                    ir::MemoryAccess(std::make_shared<ir::Temporary>(t))),
                std::make_shared<ir::Temporary>(t_n)),
            std::make_shared<ir::Label>(l_2),
            std::make_shared<ir::ConditionalJump>(  // cjump (t_n sub t_1n) l_2p
                std::make_shared<ir::BinaryExpression>(      // l_1
                    std::make_shared<ir::BinaryExpression>(  // t_n sub t_1n
                        std::make_shared<ir::Temporary>(t_n),
                        std::make_shared<ir::Temporary>(t_1n),
                        ir::BinaryOperator::Sub),
                    std::make_shared<ir::IntegerLiteral>(0),
                    ir::BinaryOperator::Neq),
                l_2p, l_1),
            std::make_shared<ir::Label>(l_2p),
            std::make_shared<ir::Assignment>(  // [t add (t_n mul 8)] ← [t2 add
                                               // (((t_n sub t1_n) mul 8) add -8)]
                ir::Destination(               // [t add (t_n mul 8)]
                    ir::MemoryAccess(std::make_shared<ir::BinaryExpression>(
                        std::make_shared<ir::Temporary>(t),
                        std::make_shared<ir::BinaryExpression>(
                            std::make_shared<ir::Temporary>(t_n),
                            std::make_shared<ir::IntegerLiteral>(8),
                            ir::BinaryOperator::Mul),
                        ir::BinaryOperator::Add))),
                std::make_shared<ir::MemoryAccess>(  // [t2 add (((t_n sub t1_n)
                                                     // mul 8) add -8)]
                    std::make_shared<ir::BinaryExpression>(
                        std::make_shared<ir::Temporary>(t_2),
                        std::make_shared<ir::BinaryExpression>(
                            std::make_shared<ir::BinaryExpression>(
                                std::make_shared<ir::BinaryExpression>(
                                    std::make_shared<ir::Temporary>(t_n),
                                    std::make_shared<ir::Temporary>(t_1n),
                                    ir::BinaryOperator::Sub),
                                std::make_shared<ir::IntegerLiteral>(8),
                                ir::BinaryOperator::Mul),
                            std::make_shared<ir::IntegerLiteral>(-8),
                            ir::BinaryOperator::Add),
                        ir::BinaryOperator::Add))),
            std::make_shared<ir::Assignment>(  // t_n <- t_n add -1
                ir::Destination(ir::Temporary(t_n)),
                std::make_shared<ir::BinaryExpression>(
                    std::make_shared<ir::Temporary>(t_n),
                    std::make_shared<ir::IntegerLiteral>(-1),
                    ir::BinaryOperator::Add)),
            std::make_shared<ir::Jump>(  // jump &l_2
                std::make_shared<ir::Address>(l_2)),
            std::make_shared<ir::Label>(l_1),
            std::make_shared<ir::ConditionalJump>(  // cjump t_n l_1p l_0
                std::make_shared<ir::BinaryExpression>(
                    std::make_shared<ir::Temporary>(t_n),
                    std::make_shared<ir::IntegerLiteral>(0),
                    ir::BinaryOperator::Neq),
                l_1p, l_0),
            std::make_shared<ir::Label>(l_1p),
            std::make_shared<ir::Assignment>(  // [t add (t_n mul 8)] ← [t2 add
                                               // ((t_n sub 1) mul 8)]
                ir::Destination(               // [t add (t_n mul 8)]
                    ir::MemoryAccess(std::make_shared<ir::BinaryExpression>(
                        std::make_shared<ir::Temporary>(t),
                        std::make_shared<ir::BinaryExpression>(
                            std::make_shared<ir::Temporary>(t_n),
                            std::make_shared<ir::IntegerLiteral>(8),
                            ir::BinaryOperator::Mul),
                        ir::BinaryOperator::Add))),
                std::make_shared<ir::MemoryAccess>(  // [t1 add ((t_n mul 8) add (-8))
                                                     // ]
                    std::make_shared<ir::BinaryExpression>(
                        std::make_shared<ir::Temporary>(t_1),
                        std::make_shared<ir::BinaryExpression>(
                            std::make_shared<ir::BinaryExpression>(
                                std::make_shared<ir::Temporary>(t_n),
                                std::make_shared<ir::IntegerLiteral>(8),
                                ir::BinaryOperator::Mul),
                            std::make_shared<ir::IntegerLiteral>(-8),
                            ir::BinaryOperator::Add),
                        ir::BinaryOperator::Add))),
            std::make_shared<ir::Assignment>(  // t_n <- t_n sub 1
                ir::Destination(ir::Temporary(t_n)),
                std::make_shared<ir::BinaryExpression>(
                    std::make_shared<ir::Temporary>(t_n),
                    std::make_shared<ir::IntegerLiteral>(-1),
                    ir::BinaryOperator::Add)),
            std::make_shared<ir::Jump>(  // jump &l_1
                std::make_shared<ir::Address>(l_1)),
            std::make_shared<ir::Label>(l_0)};
        const auto result_ptr = std::make_shared<ir::ExpressionSequence>(
            std::make_shared<ir::StatementSequence>(statements),
            std::make_shared<ir::BinaryExpression>(
                std::make_shared<ir::Temporary>(t),
                std::make_shared<ir::IntegerLiteral>(8),
                ir::BinaryOperator::Add));
        return static_cast<std::shared_ptr<ir::Expression>>(result_ptr);
      }
      throw std::runtime_error(
          std::format("unexpected operand types for +: {}, {}, probably a "
                      "problem from semantic analysis",
                      expr.left->judgement.type->toString(),
                      expr.right->judgement.type->toString()));
    }
  }
  throw std::runtime_error("unexpected binary operator");
}

std::shared_ptr<ir::Expression> allocate_array_literal(
    std::vector<std::shared_ptr<ir::Expression>> ir_exprs) {
  auto t = ir::create_temporary("t");
  int64_t n = static_cast<int64_t>(ir_exprs.size());

  const auto result_expression = std::make_shared<ir::BinaryExpression>(
      std::make_shared<ir::Temporary>(t),
      std::make_shared<ir::IntegerLiteral>(8), ir::BinaryOperator::Add);

  const auto alloc_size = static_cast<std::shared_ptr<ir::Expression>>(
      std::make_shared<ir::BinaryExpression>(
          std::make_shared<ir::BinaryExpression>(
              std::make_shared<ir::IntegerLiteral>(n),
              std::make_shared<ir::IntegerLiteral>(8), ir::BinaryOperator::Mul),
          std::make_shared<ir::IntegerLiteral>(8), ir::BinaryOperator::Add));
  const auto alloc =
      std::make_shared<ir::FunctionCall>(  // call (& eta alloc) (8n + 8)
          std::make_shared<ir::Address>("_eta_alloc"),
          std::vector<std::shared_ptr<ir::Expression>>{alloc_size});

  std::vector<std::shared_ptr<ir::Statement>> statements = {
      std::make_shared<ir::Assignment>(ir::Destination(t), alloc),
      std::make_shared<ir::Assignment>(  // [t] <- n
          ir::Destination(ir::MemoryAccess(std::make_shared<ir::Temporary>(t))),
          std::make_shared<ir::IntegerLiteral>(n))};

  for (int64_t i = 1; i <= n; i++) {
    statements.emplace_back(std::make_shared<ir::Assignment>(
        ir::Destination(ir::MemoryAccess(std::make_shared<ir::BinaryExpression>(
            std::make_shared<ir::Temporary>(t),
            std::make_shared<ir::IntegerLiteral>(8 * i),
            ir::BinaryOperator::Add))),
        ir_exprs[i - 1]));
  }
  const auto result_ptr = std::make_shared<ir::ExpressionSequence>(
      std::make_shared<ir::StatementSequence>(statements), result_expression);
  return static_cast<std::shared_ptr<ir::Expression>>(result_ptr);
}

std::any EtaASTtoIRVisitor::visit(const ast::ArrayInitializerExpression& expr)const  {
  std::vector<std::shared_ptr<ir::Expression>> ir_exprs;
  for (auto& sub_expr : expr.exprs) {
    auto sub_expr_translated =
        std::any_cast<std::shared_ptr<ir::Expression>>(sub_expr->accept(*this));
    ir_exprs.push_back(sub_expr_translated);
  }
  return static_cast<std::shared_ptr<ir::Expression>>(
      allocate_array_literal(ir_exprs));
}

std::any EtaASTtoIRVisitor::visit(const ast::StringLiteralExpression& expr)const {
  std::vector<std::shared_ptr<ir::Expression>> ir_exprs;
  ir_exprs.reserve(expr.codepoints.size());
  for (const auto codepoint : expr.codepoints) {
    ir_exprs.push_back(
        std::make_shared<ir::IntegerLiteral>(static_cast<int64_t>(codepoint)));
  }
  return static_cast<std::shared_ptr<ir::Expression>>(
      allocate_array_literal(ir_exprs));
}

std::shared_ptr<ir::Expression> callConstructor(std::vector<std::shared_ptr<ir::Expression>> arguments) {
    int n = arguments.size();
    if (n == 0) {
        return std::static_pointer_cast<ir::Expression>(std::make_shared<ir::IntegerLiteral>(0));
    }
    auto t = ir::create_temporary("t");
    std::vector<std::shared_ptr<ir::Statement>> statements = {
        std::make_shared<ir::Assignment>(t,
            std::make_shared<ir::FunctionCall>(std::make_shared<ir::Address>("_eta_alloc"),
                            std::vector<std::shared_ptr<ir::Expression>>{std::make_shared<ir::IntegerLiteral>(8 * n)}))};
    for (int i = 0; i < n; i++) {
        statements.emplace_back(std::make_shared<ir::Assignment>(
            ir::MemoryAccess(std::make_shared<ir::BinaryExpression>(
                std::make_shared<ir::Temporary>(t),
                std::make_shared<ir::IntegerLiteral>(8 * i),
                ir::BinaryOperator::Add)),
            arguments[i]));
    }
    return static_cast<std::shared_ptr<ir::Expression>>(
        std::make_shared<ir::ExpressionSequence>(
            std::make_shared<ir::StatementSequence>(statements),
            std::make_shared<ir::Temporary>(t)));
}
std::any EtaASTtoIRVisitor::visit(const ast::FunctionApplicationExpression& expr)const {
  std::vector<std::shared_ptr<ir::Expression>> arguments;
  arguments.reserve(expr.exprs.size());

  for (const auto& expression : expr.exprs) {
    const auto ir_expression = std::any_cast<std::shared_ptr<ir::Expression>>(
        expression->accept(*this));
    arguments.push_back(ir_expression);
  }

  if (expr.judgement.input.find(types::getRecordDefnAccessorStr(expr.id))) {
      return static_cast<std::shared_ptr<ir::Expression>>(callConstructor(arguments));
  }

  const auto result_ptr = std::make_shared<ir::FunctionCall>(
      std::make_shared<ir::Address>(
          ir::abi_encoding_of_function(expr.id, expr.judgement.input)),
      arguments);
  return static_cast<std::shared_ptr<ir::Expression>>(result_ptr);
}

std::any EtaASTtoIRVisitor::visit(const ast::LengthExpression& expr) const {
  const auto ir_expression =
      std::any_cast<std::shared_ptr<ir::Expression>>(expr.expr->accept(*this));
  const auto result_ptr =
      std::make_shared<ir::MemoryAccess>(std::make_shared<ir::BinaryExpression>(
          ir_expression, std::make_shared<ir::IntegerLiteral>(-8),
          ir::BinaryOperator::Add));
  return static_cast<std::shared_ptr<ir::Expression>>(result_ptr);
}

std::any EtaASTtoIRVisitor::visit(const ast::IdentifierExpression& expr)const {
  const auto entry = expr.judgement.input.find(expr.id);
  const auto variable = std::get_if<types::VariableType>(entry);
  if (variable == nullptr) {
    throw std::invalid_argument("Identifier \"" + expr.id +
                                "\" is not a variable");
  }
  if (variable->first == types::VariableCategory::Global) {
    const auto result_ptr = std::make_shared<ir::MemoryAccess>(
        std::make_shared<ir::Address>(ir::abi_encoding_of_global(expr.id)));
    return static_cast<std::shared_ptr<ir::Expression>>(result_ptr);
  } else {
    auto t = ir::temporary_of(expr.id);
    const auto result_ptr = std::make_shared<ir::Temporary>(t);
    return static_cast<std::shared_ptr<ir::Expression>>(result_ptr);
  }
}

std::any EtaASTtoIRVisitor::visit(const ast::FieldAccessExpression &expr) const {
    auto exprType = expr.expr->judgement.type;
    int idx;
    if (auto recordType = std::dynamic_pointer_cast<types::RecordType>(exprType)) {
       auto recordDefn = std::get_if<types::RecordDefnType>(
           expr.judgement.input.find(types::getRecordDefnAccessorStr(recordType->name)));
       idx = recordDefn->find(expr.id) - recordDefn->begin();
    }
    else if (std::dynamic_pointer_cast<types::NullType>(exprType) || std::dynamic_pointer_cast<types::AnyType>(exprType)) {
        idx = 0;
    }
    else {
        throw std::runtime_error("std::any EtaASTtoIRVisitor::visit(const ast::FieldAccessExpression &expr): invalid branch encountered");
    }
    const auto e =
      std::any_cast<std::shared_ptr<ir::Expression>>(expr.expr->accept(*this));
    return static_cast<std::shared_ptr<ir::Expression>>(std::make_shared<ir::MemoryAccess>(
        std::make_shared<ir::BinaryExpression>(
            e, std::make_shared<ir::IntegerLiteral>(8 * idx),
            ir::BinaryOperator::Add)));
}

std::any EtaASTtoIRVisitor::visit(const ast::BoolLiteralExpression& expr)const {
  return static_cast<std::shared_ptr<ir::Expression>>(
      std::make_shared<ir::IntegerLiteral>(expr.value ? 1 : 0));
}

std::any EtaASTtoIRVisitor::visit(const ast::NullLiteralExpression &expr) const {
    return  static_cast<std::shared_ptr<ir::Expression>>(
        std::make_shared<ir::IntegerLiteral>(0));
}

std::any EtaASTtoIRVisitor::visit(const ast::IntLiteralExpression& expr)const {
  return static_cast<std::shared_ptr<ir::Expression>>(
      std::make_shared<ir::IntegerLiteral>(expr.value));
}

std::any EtaASTtoIRVisitor::visit(const ast::CharLiteralExpression& expr) const {
  return static_cast<std::shared_ptr<ir::Expression>>(
      std::make_shared<ir::IntegerLiteral>(
          static_cast<int64_t>(expr.codepoint)));
}

// Statements
std::any EtaASTtoIRVisitor::visit(const ast::Statement& stmt)const {
  return stmt.accept(*this);
}

std::any EtaASTtoIRVisitor::visit(const ast::ReturnStatement& stmt) const {
  std::vector<std::shared_ptr<ir::Expression>> translated_exprs;
  translated_exprs.reserve(stmt.exprs.size());
  for (auto& expr : stmt.exprs) {
    translated_exprs.push_back(
        std::any_cast<std::shared_ptr<ir::Expression>>(expr->accept(*this)));
  }
  return static_cast<std::shared_ptr<ir::Statement>>(
      std::make_shared<ir::ReturnStatement>(translated_exprs));
};

/**
 * The visitor that implements the C[[e, l_t, l_f]] translation.
 */
struct ControlFlowStatementGuardVisitor : EtaASTBaseBuildVisitor {
  std::string trueLabel;
  std::string falseLabel;

  ControlFlowStatementGuardVisitor(std::string trueLabel,
                                   std::string falseLabel)
      : trueLabel(std::move(trueLabel)), falseLabel(std::move(falseLabel)) {}

  std::any default_action(const ast::Expression& expr) const {
    EtaASTtoIRVisitor visitor;
    auto translated =
        std::any_cast<std::shared_ptr<ir::Expression>>(expr.accept(visitor));
    return static_cast<std::shared_ptr<ir::Statement>>(
        std::make_shared<ir::ConditionalJump>(translated, trueLabel,
                                              falseLabel));
  }

  std::any visit(const ast::Expression& expr)const  override { return expr.accept(*this); }

  std::any visit(const ast::UnaryExpression& expr) const override {
    if (expr.op == ast::Expression::UnaryOperand::Bang) {
      ControlFlowStatementGuardVisitor visitor(falseLabel, trueLabel);
      return std::any_cast<std::shared_ptr<ir::Statement>>(
          expr.expr->accept(visitor));
    }
    return default_action(expr);
  }

  std::any visit(const ast::BinaryExpression& expr) const override {
    std::string lt = ir::create_label_name("lt");
    if (expr.op != ast::Expression::BinaryOperand::And &&
        expr.op != ast::Expression::BinaryOperand::Or) {
      return default_action(expr);
    }
    ControlFlowStatementGuardVisitor visitor_left =
        expr.op == ast::Expression::BinaryOperand::And
            ? ControlFlowStatementGuardVisitor(lt, falseLabel)
            : ControlFlowStatementGuardVisitor(trueLabel, lt);
    ControlFlowStatementGuardVisitor visitor_right =
        ControlFlowStatementGuardVisitor(trueLabel, falseLabel);
    ir::StatementSequence statement({
        std::any_cast<std::shared_ptr<ir::Statement>>(
            expr.left->accept(visitor_left)),
        std::make_shared<ir::Label>(lt),
        std::any_cast<std::shared_ptr<ir::Statement>>(
            expr.right->accept(visitor_right)),
    });
    return static_cast<std::shared_ptr<ir::Statement>>(
        std::make_shared<ir::StatementSequence>(statement));
  }

  std::any visit(const ast::BoolLiteralExpression& expr) const override {
    std::string jumpTo = expr.value ? trueLabel : falseLabel;
    auto jump =
        std::make_shared<ir::Jump>(std::make_shared<ir::Address>(jumpTo));
    return static_cast<std::shared_ptr<ir::Statement>>(jump);
  }

  std::any visit(const ast::IntLiteralExpression& expr)const  override {
    return default_action(expr);
  }

  std::any visit(const ast::ArrayInitializerExpression& expr) const override {
    return default_action(expr);
  }

  std::any visit(const ast::LengthExpression& expr)const  override {
    return default_action(expr);
  }

  std::any visit(const ast::IdentifierExpression& expr) const override {
    return default_action(expr);
  }

  std::any visit(const ast::StringLiteralExpression& expr) const override {
    return default_action(expr);
  }

  std::any visit(const ast::CharLiteralExpression& expr) const override {
    return default_action(expr);
  }

  std::any visit(const ast::FunctionApplicationExpression& expr) const override {
    return default_action(expr);
  }

  std::any visit(const ast::FieldAccessExpression& expr) const override {
      return default_action(expr);
  }

    std::any visit(const ast::NullLiteralExpression& expr) const override {
      return default_action(expr);
  }
};

std::any EtaASTtoIRVisitor::visit(const ast::IfStatement& stmt)const  {
  std::string trueLabel = ir::create_label_name("l_true");
  std::string falseLabel = ir::create_label_name("l_false");
  std::vector<std::shared_ptr<ir::Statement>> sequence;
  ControlFlowStatementGuardVisitor visitor(trueLabel, falseLabel);
  if (stmt.else_branch == nullptr) {
    sequence = {std::any_cast<std::shared_ptr<ir::Statement>>(
                    stmt.condition->accept(visitor)),
                std::make_shared<ir::Label>(trueLabel),
                std::any_cast<std::shared_ptr<ir::Statement>>(
                    stmt.then_branch->accept(*this)),
                std::make_shared<ir::Label>(falseLabel)};
  } else {
    std::string doneLabel = ir::create_label_name("l_done");
    sequence = {
        std::any_cast<std::shared_ptr<ir::Statement>>(
            stmt.condition->accept(visitor)),
        std::make_shared<ir::Label>(trueLabel),
        std::any_cast<std::shared_ptr<ir::Statement>>(
            stmt.then_branch->accept(*this)),
        std::make_shared<ir::Jump>(std::make_shared<ir::Address>(doneLabel)),
        std::make_shared<ir::Label>(falseLabel),
        std::any_cast<std::shared_ptr<ir::Statement>>(
            stmt.else_branch->accept(*this)),
        std::make_shared<ir::Label>(doneLabel)};
  }
  return static_cast<std::shared_ptr<ir::Statement>>(
      std::make_shared<ir::StatementSequence>(sequence));
};

std::any EtaASTtoIRVisitor::visit(const ast::WhileStatement& stmt)const {
  std::string startLabel = ir::create_label_name("start");
  std::string bodyLabel = ir::create_label_name("body");
  std::string endLabel = ir::create_label_name("end");
  auto old_continuation = continuation;
  continuation = endLabel;
  ControlFlowStatementGuardVisitor visitor(bodyLabel, endLabel);
  std::vector<std::shared_ptr<ir::Statement>> sequence(
      {std::make_shared<ir::Label>(startLabel),
       std::any_cast<std::shared_ptr<ir::Statement>>(
           stmt.condition->accept(visitor)),
       std::make_shared<ir::Label>(bodyLabel),
       std::any_cast<std::shared_ptr<ir::Statement>>(stmt.body->accept(*this)),
       std::make_shared<ir::Jump>(std::make_shared<ir::Address>(startLabel)),
       std::make_shared<ir::Label>(endLabel)});
  continuation = old_continuation;
  return static_cast<std::shared_ptr<ir::Statement>>(
      std::make_shared<ir::StatementSequence>(sequence));
};

std::any EtaASTtoIRVisitor::visit(const ast::BreakStatement &stmt) const {
    if (!continuation.has_value()) {
        throw std::runtime_error("std::any EtaASTtoIRVisitor::visit(const ast::BreakStatement &stmt): unreachable branch encountere");
    }
    return static_cast<std::shared_ptr<ir::Statement>>(
        std::make_shared<ir::Jump>(std::make_shared<ir::Address>(continuation.value())));
}

std::any EtaASTtoIRVisitor::visit(const ast::BlockStatement& stmt)const {
  std::vector<std::shared_ptr<ir::Statement>> sequence;
  sequence.reserve(stmt.statements.size());
  for (auto& statement : stmt.statements) {
    sequence.push_back(std::any_cast<std::shared_ptr<ir::Statement>>(
        statement->accept(*this)));
  }
  return static_cast<std::shared_ptr<ir::Statement>>(
      std::make_shared<ir::StatementSequence>(sequence));
};

std::any EtaASTtoIRVisitor::visit(const ast::ProcedureCall& stmt) const {
  std::vector<std::shared_ptr<ir::Expression>> translated_args;
  translated_args.reserve(stmt.arguments.size());
  for (auto& arg : stmt.arguments) {
    translated_args.push_back(
        std::any_cast<std::shared_ptr<ir::Expression>>(arg->accept(*this)));
  }
  if (auto* ptr =
          std::get_if<types::MethodType>(stmt.judgement.input.find(stmt.id))) {
    return static_cast<std::shared_ptr<ir::Statement>>(
        std::make_shared<ir::CallStatement>(
            std::make_shared<ir::Address>(
                ir::abi_encoding_of_function(stmt.id, stmt.judgement.input)),
            translated_args, ptr->second.size()));
  } else
    throw std::runtime_error(
        " EtaASTtoIRVisitor::visit(ast::ProcedureCall &stmt): impossible "
        "branch encountered");
};

std::shared_ptr<ir::Expression> allocate(std::vector<ir::Temporary> exprs) {
    if (exprs.empty()) {
        return std::make_shared<ir::IntegerLiteral>(0);
    }
    auto head = *exprs.begin();
    exprs.erase(exprs.begin());
     auto t_counter = ir::create_temporary("t_counter");
    auto t_arr = ir::create_temporary("t_arr");
    EtaASTtoIRVisitor expression_visitor;
    std::string l_start = ir::create_label_name("l_start");
    std::string l_body = ir::create_label_name("l_body");
    std::string l_end = ir::create_label_name("l_end");
    std::vector<std::shared_ptr<ir::Statement>> sequence = {
        std::make_shared<ir::Assignment>(t_counter,
                                         std::make_shared<ir::Temporary>(head)),
        std::make_shared<ir::Assignment>(
            t_arr, std::make_shared<ir::FunctionCall>(
                       std::make_shared<ir::Address>("_eta_alloc"),
                       std::vector<std::shared_ptr<ir::Expression>>(
                           {std::make_shared<ir::BinaryExpression>(
                               std::make_shared<ir::BinaryExpression>(
                                   std::make_shared<ir::Temporary>(head),
                                   std::make_shared<ir::IntegerLiteral>(8),
                                   ir::BinaryOperator::Mul),
                               std::make_shared<ir::IntegerLiteral>(8),
                               ir::BinaryOperator::Add)}))),
        std::make_shared<ir::Assignment>(
            ir::MemoryAccess(std::make_shared<ir::Temporary>(t_arr)),
            std::make_shared<ir::Temporary>(head)),
        std::make_shared<ir::Label>(l_start),
        std::make_shared<ir::ConditionalJump>(
            std::make_shared<ir::BinaryExpression>(
                std::make_shared<ir::Temporary>(t_counter),
                std::make_shared<ir::IntegerLiteral>(0),
                ir::BinaryOperator::Neq),
            l_body, l_end),
        std::make_shared<ir::Label>(l_body),
        std::make_shared<ir::Assignment>(
            ir::MemoryAccess(std::make_shared<ir::BinaryExpression>(
                std::make_shared<ir::Temporary>(t_arr),
                std::make_shared<ir::BinaryExpression>(
                    std::make_shared<ir::Temporary>(t_counter),
                    std::make_shared<ir::IntegerLiteral>(8),
                    ir::BinaryOperator::Mul),
                ir::BinaryOperator::Add)),
           allocate(exprs)),
        std::make_shared<ir::Assignment>(
            t_counter, std::make_shared<ir::BinaryExpression>(
                           std::make_shared<ir::Temporary>(t_counter),
                           std::make_shared<ir::IntegerLiteral>(-1),
                           ir::BinaryOperator::Add)),
        std::make_shared<ir::Jump>(std::make_shared<ir::Address>(l_start)),
        std::make_shared<ir::Label>(l_end)};
    return std::make_shared<ir::ExpressionSequence>(
        std::make_shared<ir::StatementSequence>(sequence),
        std::make_shared<ir::BinaryExpression>(
            std::make_shared<ir::Temporary>(t_arr),
            std::make_shared<ir::IntegerLiteral>(8), ir::BinaryOperator::Add));
}

// Type nodes
std::any EtaASTtoIRVisitor::visit(const ast::EtaTypeNode& type_node)const {
  return type_node.accept(*this);
};

std::any EtaASTtoIRVisitor::visit(const ast::BaseTypeNode& type_node)const {
  return static_cast<std::shared_ptr<ir::Expression>>(
      std::make_shared<ir::IntegerLiteral>(0));
};

std::any EtaASTtoIRVisitor::visit(const ast::RecordTypeNode &type_node) const {
    return static_cast<std::shared_ptr<ir::Expression>>(
      std::make_shared<ir::IntegerLiteral>(0));
}

std::any EtaASTtoIRVisitor::visit(const ast::ArrayTypeNode& type_node)const {
    std::vector<ir::Temporary> temps;
    std::vector<std::shared_ptr<ir::Statement>> sequence;
    for (int i = 0; i < type_node.allocators.size(); i++) {
        auto e_i = type_node.allocators[i];
        auto t_i = ir::create_temporary(std::format("t_{}", i));
        temps.push_back(t_i);
        auto e_i_translated = std::any_cast<std::shared_ptr<ir::Expression>>(e_i->accept(*this));
        sequence.push_back(std::make_shared<ir::Assignment>(t_i, e_i_translated));
    }
    return static_cast<std::shared_ptr<ir::Expression>>(std::make_shared<ir::ExpressionSequence>(
        std::make_shared<ir::StatementSequence>(sequence), allocate(temps)));
};

std::any EtaASTtoIRVisitor::visit(const ast::DeclarationStatement& stmt)const {
  std::vector<std::shared_ptr<ir::Statement>> sequence;
  sequence.reserve(stmt.declarations.size());
  for (auto& declaration : stmt.declarations) {
    auto translated_dim = std::any_cast<std::shared_ptr<ir::Expression>>(
        declaration.second->accept(*this));
    sequence.emplace_back(std::make_shared<ir::Assignment>(
        ir::temporary_of(declaration.first), translated_dim));
  }
  return static_cast<std::shared_ptr<ir::Statement>>(
      std::make_shared<ir::StatementSequence>(sequence));
};

// Lvalue nodes
std::any EtaASTtoIRVisitor::visit(const ast::LvalueNode& lvalue_node)const  {
  return lvalue_node.accept(*this);
};

std::any EtaASTtoIRVisitor::visit(const ast::WildcardLvalueNode& node)const  {
  return ir::Destination(ir::Temporary("_W"));
};

std::any EtaASTtoIRVisitor::visit(const ast::DeclarationLvalueNode& node)const {
  return ir::Destination(
      ir::Temporary(ir::temporary_of(node.declaration->first)));
};

std::any EtaASTtoIRVisitor::visit(const ast::LvalueRecordNode &lvalue_node) const {
    const auto exprType = lvalue_node.record->judgement.type;
    int idx;
    if (auto recordType = std::dynamic_pointer_cast<types::RecordType>(exprType)) {
        auto recordDefn = std::get_if<types::RecordDefnType>(
            lvalue_node.judgement.input.find(types::getRecordDefnAccessorStr(recordType->name)));
        idx = recordDefn->find(lvalue_node.id) - recordDefn->begin();
    }
    else if (std::dynamic_pointer_cast<types::NullType>(exprType) || std::dynamic_pointer_cast<types::AnyType>(exprType)) {
        idx = 0;
    }
    else {
        throw std::runtime_error("std::any EtaASTtoIRVisitor::visit(const ast::FieldAccessExpression &expr): invalid branch encountered");
    }
    const auto e =
      std::any_cast<std::shared_ptr<ir::Expression>>(lvalue_node.record->accept(*this));
    return ir::Destination(ir::MemoryAccess(
        std::make_shared<ir::BinaryExpression>(
            e, std::make_shared<ir::IntegerLiteral>(8 * idx),
            ir::BinaryOperator::Add)));
}

std::any EtaASTtoIRVisitor::visit(const ast::LvalueIdentifierNode& node) const {
  if (auto* ptr = std::get_if<types::VariableType>(
          node.judgement.input.find(node.id))) {
    switch (ptr->first) {
      case types::VariableCategory::Local:
        return ir::Destination(ir::Temporary(ir::temporary_of(node.id)));
      case types::VariableCategory::Global:
        return ir::Destination(ir::MemoryAccess(std::make_shared<ir::Address>(
            ir::abi_encoding_of_global(node.id))));
    }
  }
  throw std::runtime_error(
      "EtaASTtoIRVisitor::visit(ast::LvalueIdentifierNode &node): "
      "impossible branch encountered");
};

std::any EtaASTtoIRVisitor::visit(const ast::LvalueArrayNode& node) const {
  const auto e1 = std::any_cast<std::shared_ptr<ir::Expression>>(
      node.arrayIndexExpression->left->accept(*this));
  const auto e2 = std::any_cast<std::shared_ptr<ir::Expression>>(
      node.arrayIndexExpression->right->accept(*this));
  auto t_a = ir::create_temporary("t_a");
  auto t_i = ir::create_temporary("t_i");
  auto l_err = ir::create_label_name("l_err");
  auto l_ok = ir::create_label_name("l_ok");
  std::vector<std::shared_ptr<ir::Statement>> statements = {
      std::make_shared<ir::Assignment>(  // t_a <- e1
          ir::Destination(t_a), e1),
      std::make_shared<ir::Assignment>(  // t_i <- e2
          ir::Destination(t_i), e2),
      std::make_shared<ir::ConditionalJump>(  // cjump (t_i ult [t_a sub 8])
                                              // l_ok l_err
          std::make_shared<ir::BinaryExpression>(  // t_i ult [t_a sub 8]
              std::make_shared<ir::Temporary>(t_i),
              std::make_shared<ir::MemoryAccess>(  // [t_a sub 8]
                  std::make_shared<ir::BinaryExpression>(
                      std::make_shared<ir::Temporary>(t_a),
                      std::make_shared<ir::IntegerLiteral>(8),
                      ir::BinaryOperator::Sub)),
              ir::BinaryOperator::Ult),
          l_ok, l_err),
      std::make_shared<ir::Label>(l_err),
      std::make_shared<ir::CallStatement>(
          std::make_shared<ir::Address>("_eta_out_of_bounds"),
          std::vector<std::shared_ptr<ir::Expression>>{}, 0),
      std::make_shared<ir::Label>(l_ok)};
  return ir::Destination(
      ir::MemoryAccess(std::make_shared<ir::ExpressionSequence>(
          std::make_shared<ir::StatementSequence>(
              statements),  // [t_a add (t_i mul 8)]
          std::make_shared<ir::BinaryExpression>(
              std::make_shared<ir::Temporary>(t_a),
              std::make_shared<ir::BinaryExpression>(
                  std::make_shared<ir::Temporary>(t_i),
                  std::make_shared<ir::IntegerLiteral>(8),
                  ir::BinaryOperator::Mul),
              ir::BinaryOperator::Add))));
};

inline std::shared_ptr<ir::Temporary> temp_of_return_value(int n) {
  // 1-indexing, what the fuck???
  return std::make_shared<ir::Temporary>(std::format("_RV{}", n + 1));
}

std::any EtaASTtoIRVisitor::visit(const ast::AssignmentStatement& stmt) const {
  std::vector<std::shared_ptr<ir::Statement>> sequence;
  sequence.reserve(stmt.lvalues.size() + 1);
  if (stmt.rvalues.size() == 1 &&
      dynamic_cast<ast::FunctionApplicationExpression*>(
          stmt.rvalues.front().get()) != nullptr) {
    auto* funPtr = dynamic_cast<ast::FunctionApplicationExpression*>(
        stmt.rvalues.front().get());
    std::vector<std::shared_ptr<ir::Expression>> translated_args;
    translated_args.reserve(funPtr->exprs.size());
    for (auto& arg : funPtr->exprs) {
      translated_args.push_back(
          std::any_cast<std::shared_ptr<ir::Expression>>(arg->accept(*this)));
    }

    if (stmt.judgement.input.find(types::getRecordDefnAccessorStr(funPtr->id))) {
        return static_cast<std::shared_ptr<ir::Statement>>(
            std::make_shared<ir::Assignment>(std::any_cast<ir::Destination>(stmt.lvalues[0]->accept(*this)),
                callConstructor(translated_args)));
    }

    if (auto* ptr = std::get_if<types::MethodType>(
            stmt.judgement.input.find(funPtr->id))) {
      auto n_returns = ptr->second.size();
      sequence.push_back(std::make_shared<ir::CallStatement>(
          std::make_shared<ir::Address>(
              ir::abi_encoding_of_function(funPtr->id, stmt.judgement.input)),
          translated_args, n_returns));
    } else
      throw std::runtime_error(
          "EtaASTtoIRVisitor::visit(ast::AssignmentStatement &stmt): "
          "impossible branch encountered");
    for (int i = 0; i < stmt.lvalues.size(); i++) {
      auto& lvalue = stmt.lvalues[i];
      sequence.push_back(std::make_shared<ir::Assignment>(
          std::any_cast<ir::Destination>(lvalue->accept(*this)),
          temp_of_return_value(i)));
    }
  } else {
    std::vector<ir::Temporary> temps;
    temps.reserve(stmt.lvalues.size());
    for (int i = 0; i < stmt.lvalues.size(); i++) {
      temps.push_back(ir::create_temporary(std::format("t{}", i)));
    }
    for (int i = 0; i < stmt.rvalues.size(); i++) {
      auto& rvalue = stmt.rvalues[i];
      sequence.push_back(std::make_shared<ir::Assignment>(
          temps[i], std::any_cast<std::shared_ptr<ir::Expression>>(
                        rvalue->accept(*this))));
    }
    for (int i = 0; i < stmt.lvalues.size(); i++) {
      auto& lvalue = stmt.lvalues[i];
      sequence.push_back(std::make_shared<ir::Assignment>(
          std::any_cast<ir::Destination>(lvalue->accept(*this)),
          std::make_shared<ir::Temporary>(temps[i])));
    }
  }
  return static_cast<std::shared_ptr<ir::Statement>>(
      std::make_shared<ir::StatementSequence>(sequence));
};

std::any EtaASTtoIRVisitor::visit(const ast::Definition &def) const {
    return def.accept(*this);
}

// Declarations
std::any EtaASTtoIRVisitor::visit(const ast::GlobalVariableDeclaration& decl)const {
  if (decl.value != nullptr) {
    auto translated_expr = std::any_cast<std::shared_ptr<ir::Expression>>(
        decl.value->accept(*this));
    auto casted_expr =
        std::dynamic_pointer_cast<ir::IntegerLiteral>(translated_expr);
    if (!casted_expr) {
      throw std::runtime_error(
          "EtaASTtoIRVisitor::visit(ast::GlobalDeclaration &decl): impossible "
          "branch encountered");
    }
    return ir::DataDeclaration(
        ir::abi_encoding_of_global(decl.declaration->first),
        std::vector<int64_t>({casted_expr->value}));
  } else {
    return ir::DataDeclaration(
        ir::abi_encoding_of_global(decl.declaration->first),
        std::vector<int64_t>({0}));
  }
};
std::any EtaASTtoIRVisitor::visit(const ast::MethodInterface& method_interface)const {
  throw std::runtime_error(
      "EtaASTtoIRVisitor::visit(ast::MethodInterface &method_interface): entry "
      "point unavailable");
};

inline std::shared_ptr<ir::Temporary> temp_of_argument(int n) {
  // 1-indexing, what the fuck???
  return std::make_shared<ir::Temporary>(std::format("_ARG{}", n + 1));
}

std::any EtaASTtoIRVisitor::visit(const ast::Method& method)const {
  std::vector<std::shared_ptr<ir::Statement>> translated_preamble;
  translated_preamble.reserve(method.header->declarations.size() + 1);
  for (int i = 0; i < method.header->declarations.size(); i++) {
    auto& arg = method.header->declarations[i];
    translated_preamble.push_back(std::make_shared<ir::Assignment>(
        ir::temporary_of(arg.first), temp_of_argument(i)));
  }
  const auto translated_body =
      std::any_cast<std::shared_ptr<ir::Statement>>(method.body->accept(*this));

  translated_preamble.push_back(translated_body);

  // no return values, procedure call, must insert an empty return
  if (method.header->return_types.empty()) {
    translated_preamble.emplace_back(std::make_shared<ir::ReturnStatement>(
        std::vector<std::shared_ptr<ir::Expression>>{}));
  }
  return ir::FunctionDeclaration(
      ir::abi_encoding_of_function(method.header->id,
                                   method.secondJudgement.output),
      std::make_shared<ir::StatementSequence>(translated_preamble),
      method.header->declarations.size(),
      method.header->return_types.size()
        );
};

// Top-level
std::any EtaASTtoIRVisitor::visit(const ast::Program& program)const {
  if (program.name.empty()) {
    throw std::runtime_error(
        "program name not specified. Somebody needs to specify that in the "
        "driver. ");
  }
  std::vector<ir::DataDeclaration> data_declarations;
  std::vector<ir::FunctionDeclaration> function_declarations;
  for (auto& definition : program.definitions) {
    if (auto* const ptr_method = dynamic_cast<ast::Method *>(definition.get())) {
      auto translated_method =
          std::any_cast<ir::FunctionDeclaration>(ptr_method->accept(*this));
      function_declarations.push_back(translated_method);
    } else if (auto* const ptr_gd = dynamic_cast<ast::GlobalVariableDeclaration *>(definition.get())) {
      auto translated_declaration =
          std::any_cast<ir::DataDeclaration>(ptr_gd->accept(*this));
      data_declarations.push_back(translated_declaration);
    }
  }
  return ir::CompilationUnit(program.name, data_declarations,
                             function_declarations);
};
}  // namespace ssy38::my_compiler::visitor
#pragma clang diagnostic pop
