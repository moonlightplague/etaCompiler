#include "EtaIRtoASMVisitor.hpp"

#include <bit>
#include <cctype>
#include <format>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <variant>

#include "ir.hpp"
#include "utils.hpp"

namespace ssy38::my_compiler::visitor {
/**
 * Computes the location of the i'th argument to a function.
 *
 * @param i - the position of the argument (1-indexing)
 * @return the operand corresponding of the ith argument to a function
 */
std::shared_ptr<codegen::Operand> loc_of_arg(const std::size_t i) {
  if (i == 0) {
    throw std::runtime_error("loc_of_arg function only accepts integer > 0");
  } else if (i <= 6) {
    switch (i) {
      case 1:
        return std::make_shared<codegen::RegisterOperand>(
            codegen::Register::RDI);
      case 2:
        return std::make_shared<codegen::RegisterOperand>(
            codegen::Register::RSI);
      case 3:
        return std::make_shared<codegen::RegisterOperand>(
            codegen::Register::RDX);
      case 4:
        return std::make_shared<codegen::RegisterOperand>(
            codegen::Register::RCX);
      case 5:
        return std::make_shared<codegen::RegisterOperand>(
            codegen::Register::R8);
      case 6:
        return std::make_shared<codegen::RegisterOperand>(
            codegen::Register::R9);
      default:
        throw std::logic_error("Unexpected argument index in loc_of_arg");
    }
  } else {
    return std::make_shared<codegen::MemoryOperand>(
        codegen::Size::QWORD_PTR,
        codegen::RegisterOperand(codegen::Register::RBP), std::nullopt,
        std::nullopt,
        codegen::ConstantOperand(static_cast<int64_t>(8 * (i - 5))));
  }
}

/**
 * Computes the location corresponding to the ith return value to a function.
 * @param i - the position of the return value (1-indexing).
 * @return the operand corresponding of the ith return value to a function
 */
std::shared_ptr<codegen::Operand> loc_of_rv(const std::size_t i) {
  if (i == 0) {
    throw std::runtime_error("loc_of_arg function only accepts integer > 0");
  } else if (i <= 2) {
    switch (i) {
      case 1:
        return std::make_shared<codegen::RegisterOperand>(
            codegen::Register::RAX);
      case 2:
        return std::make_shared<codegen::RegisterOperand>(
            codegen::Register::RDX);
      default:
        throw std::logic_error("Unexpected argument index in loc_of_rv");
    }
  } else {
    return std::make_shared<codegen::MemoryOperand>(
        codegen::Size::QWORD_PTR, codegen::TemporaryOperand("_RVLOC"),
        std::nullopt, std::nullopt,
        codegen::ConstantOperand(static_cast<int64_t>(8 * (i - 3))));
  }
}

std::any EtaIRtoASMVisitor::visit(ir::IRNode& node) {
  return node.accept(*this);
}

std::any EtaIRtoASMVisitor::visit(ir::DataDeclaration& node) {
  std::vector<std::shared_ptr<codegen::Command>> result{
      std::make_shared<codegen::Directive>(".data"),
      std::make_shared<codegen::Directive>(
          std::format(".type {}, @object", node.name)),
      std::make_shared<codegen::Directive>(
          std::format(".size {}, {}", node.name, 8 * node.data.size())),
      std::make_shared<codegen::LabelDeclaration>(node.name),
  };
  for (const auto& v : node.data) {
    result.push_back(
        std::make_shared<codegen::Directive>(std::format(".quad {}", v)));
  }
  return codegen::CommandSequence(result);
}

std::any EtaIRtoASMVisitor::visit(ir::FunctionDeclaration& node) {
  std::vector<std::shared_ptr<codegen::Command>> result{
      std::make_shared<codegen::Directive>(".text"),
      std::make_shared<codegen::Directive>(std::format(".globl {}", node.name)),
      std::make_shared<codegen::Directive>(
          std::format(".type {}, @function", node.name)),
      std::make_shared<codegen::LabelDeclaration>(node.name),
      std::make_shared<codegen::Operation>(
          codegen::OpCode::ENTER,
          std::make_shared<codegen::TemporaryOperand>(
              std::format("FRAME_SIZE_OF_{}", node.name)),
          std::make_shared<codegen::ConstantOperand>(0))};
  if (node.return_val_count <= 2) {
    for (size_t i = 1; i <= node.arg_count; i++) {
      result.push_back(std::make_shared<codegen::Operation>(
          codegen::OpCode::MOV,
          std::make_shared<codegen::TemporaryOperand>(std::format("_ARG{}", i)),
          loc_of_arg(i)));
    }
  } else {
    result.push_back(std::make_shared<codegen::Operation>(
        codegen::OpCode::MOV,
        std::make_shared<codegen::TemporaryOperand>("_RVLOC"), loc_of_arg(1)));
    for (size_t i = 1; i <= node.arg_count; i++) {
      result.push_back(std::make_shared<codegen::Operation>(
          codegen::OpCode::MOV,
          std::make_shared<codegen::TemporaryOperand>(std::format("_ARG{}", i)),
          loc_of_arg(i + 1)));
    }
  }

  // save callee-save registers
  for (const auto& [temp, reg] : temps_to_csr) {
    result.push_back(std::make_shared<codegen::Operation>(codegen::OpCode::MOV,
      std::make_shared<codegen::TemporaryOperand>(temp),
      std::make_shared<codegen::RegisterOperand>(reg)));
  }

  auto body_asm =
      std::any_cast<codegen::CommandSequence>(node.body->accept(*this));
  result.insert(result.end(), body_asm.commands.begin(),
                body_asm.commands.end());
  result.push_back(std::make_shared<codegen::Directive>(
      std::format(".size {}, .-{}", node.name, node.name)));
  // Indicate where is the end of a function (Register allocation helper, would
  // be deleted later)
  result.push_back(std::make_shared<codegen::LabelDeclaration>("END_OF_FUNC"));
  return codegen::CommandSequence(result);
}

std::any EtaIRtoASMVisitor::visit(ir::CompilationUnit& node) {
  std::vector<std::shared_ptr<codegen::Command>> result{
      std::make_shared<codegen::Directive>(".intel_syntax noprefix")};
  for (auto& dataDeclaration : node.dataDeclarations) {
    auto v =
        std::any_cast<codegen::CommandSequence>(dataDeclaration.accept(*this));
    result.insert(result.end(), v.commands.begin(), v.commands.end());
  }
  for (auto& functionDeclaration : node.functionDeclarations) {
    auto v = std::any_cast<codegen::CommandSequence>(
        functionDeclaration.accept(*this));
    result.insert(result.end(), v.commands.begin(), v.commands.end());
  }
  std::vector<std::shared_ptr<codegen::Command>> security_block{
      std::make_shared<codegen::Directive>(
          ".ident \"GCC: (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0\""),
      std::make_shared<codegen::Directive>(
          ".section .note.GNU-stack,\"\",@progbits"),
      std::make_shared<codegen::Directive>(".section .note.gnu.property,\"a\""),
      std::make_shared<codegen::Directive>(".align 8"),
      std::make_shared<codegen::Directive>(".long 1f - 0f"),
      std::make_shared<codegen::Directive>(".long 4f - 1f"),
      std::make_shared<codegen::Directive>(".long	5"),
      std::make_shared<codegen::LabelDeclaration>("0"),
      std::make_shared<codegen::Directive>(".string \"GNU\""),
      std::make_shared<codegen::LabelDeclaration>("1"),
      std::make_shared<codegen::Directive>(".align 8"),
      std::make_shared<codegen::Directive>(".long	0xc0000002"),
      std::make_shared<codegen::Directive>(".long	3f - 2f"),
      std::make_shared<codegen::LabelDeclaration>("2"),
      std::make_shared<codegen::Directive>(".long	0x3"),
      std::make_shared<codegen::LabelDeclaration>("3"),
      std::make_shared<codegen::Directive>(".align 8"),
      std::make_shared<codegen::LabelDeclaration>("4"),
  };
  result.insert(result.end(), security_block.begin(), security_block.end());
  return codegen::CommandSequence(result);
}

// Expressions

std::any EtaIRtoASMVisitor::visit(ir::Expression& node) {
  throw std::runtime_error(
      "Visiting expressions inside EtaIRtoASMVisitor is prohibited, use "
      "EtaASMTemporaryAllocatorVisitor instead.");
}

std::any EtaIRtoASMVisitor::visit(ir::IntegerLiteral& node) {
  throw std::runtime_error(
      "Visiting expressions inside EtaIRtoASMVisitor is prohibited, use "
      "EtaASMTemporaryAllocatorVisitor instead.");
}

std::any EtaIRtoASMVisitor::visit(ir::Temporary& node) {
  throw std::runtime_error(
      "Visiting expressions inside EtaIRtoASMVisitor is prohibited, use "
      "EtaASMTemporaryAllocatorVisitor instead.");
}

std::any EtaIRtoASMVisitor::visit(ir::BinaryExpression& node) {
  throw std::runtime_error(
      "Visiting expressions inside EtaIRtoASMVisitor is prohibited, use "
      "EtaASMTemporaryAllocatorVisitor instead.");
}

std::any EtaIRtoASMVisitor::visit(ir::MemoryAccess& node) {
  throw std::runtime_error(
      "Visiting expressions inside EtaIRtoASMVisitor is prohibited, use "
      "EtaASMTemporaryAllocatorVisitor instead.");
}

std::any EtaIRtoASMVisitor::visit(ir::FunctionCall& node) {
  throw std::runtime_error(
      "Function call expression shouldn't exist after IR lowering.");
}

std::any EtaIRtoASMVisitor::visit(ir::Address& node) {
  throw std::runtime_error(
      "Visiting expressions inside EtaIRtoASMVisitor is prohibited, use "
      "EtaASMTemporaryAllocatorVisitor instead.");
}

std::any EtaIRtoASMVisitor::visit(ir::ExpressionSequence& node) {
  throw std::runtime_error(
      "Expression sequence shouldn't exist after IR lowering.");
}

// Statements

/**
 * Checks whether an integer could be used as the scale of a memory operand.
 *
 * @param c - the integer to be tested on
 * @return whether c could be used as the scale of a memory operand
 */
inline bool is_valid_lea_scale(const std::shared_ptr<ir::IntegerLiteral>& c) {
  return c != nullptr &&
         (c->value == 1 || c->value == 2 || c->value == 4 || c->value == 8);
}

/**
 * Checks whether an 64-bit integer is within the range of a 32-bit integer constant.
 *
 * @param c the integer to be tested on
 * @return whether c is within the range of a 32-bit integer
 */
inline bool is_int32_constant(const std::shared_ptr<ir::IntegerLiteral>& c) {
  return c != nullptr && c->value >= std::numeric_limits<int32_t>::min() &&
         c->value <= std::numeric_limits<int32_t>::max();
}

// Helper structs to test if a binary expression is part of a pattern

/**
 * Matches a multiplication by a valid LEA scale (e * n).
 * Identifies the `Index * Scale` portion of an x86-64 LEA instruction.
 */
struct MulByScalePattern {
  /** True if the IR expression matches the target pattern. */
  bool matched = false;

  /** The base expression being multiplied (e). */
  std::shared_ptr<ir::Expression> expr = nullptr;

  /** The integer literal acting as the scale (n). */
  std::shared_ptr<ir::IntegerLiteral> scale = nullptr;

  /**
   * Attempts to match the pattern against a binary expression.
   * @param binary - the IR binary expression to analyze.
   */
  explicit MulByScalePattern(
      const std::shared_ptr<ir::BinaryExpression>& binary) {
    if (!binary || binary->op != ir::BinaryOperator::Mul) {
      return;
    }

    auto candidate_scale =
        std::dynamic_pointer_cast<ir::IntegerLiteral>(binary->right);
    if (!is_valid_lea_scale(candidate_scale)) {
      return;
    }
    matched = true;
    expr = binary->left;
    scale = candidate_scale;
  }
};

/**
 * Matches an addition with a 32-bit constant (e + n).
 * Identifies the `Base + Displacement` portion of an LEA instruction.
 */
struct AddConstPattern {
  /** True if the IR expression matches the target pattern. */
  bool matched = false;

  /** The left-hand expression (e). */
  std::shared_ptr<ir::Expression> expr = nullptr;

  /** The 32-bit integer constant displacement (n). */
  std::shared_ptr<ir::IntegerLiteral> constant = nullptr;

  /**
   * Attempts to match the pattern against a binary expression.
   * @param binary - the IR binary expression to analyze.
   */
  explicit AddConstPattern(
      const std::shared_ptr<ir::BinaryExpression>& binary) {
    if (!binary || binary->op != ir::BinaryOperator::Add) {
      return;
    }

    auto candidate_constant =
        std::dynamic_pointer_cast<ir::IntegerLiteral>(binary->right);
    auto left_binary =
        std::dynamic_pointer_cast<ir::BinaryExpression>(binary->left);
    if (!is_int32_constant(candidate_constant) || left_binary != nullptr) {
      return;
    }
    matched = true;
    expr = binary->left;
    constant = candidate_constant;
  }
};

/**
 * Matches a scaled multiplication added to a constant (e * n1 + n2).
 * Identifies the `(Index * Scale) + Displacement` combination.
 */
struct MulScaleAddConstPattern {
  /** True if the IR expression matches the target pattern. */
  bool matched = false;

  /** The expression being scaled (e). */
  std::shared_ptr<ir::Expression> expr = nullptr;

  /** The valid scale multiplier (n1). */
  std::shared_ptr<ir::IntegerLiteral> scale = nullptr;

  /** The 32-bit integer constant displacement (n2). */
  std::shared_ptr<ir::IntegerLiteral> constant = nullptr;

  /**
   * Attempts to match the pattern against a binary expression.
   * @param binary - the IR binary expression to analyze.
   */
  explicit MulScaleAddConstPattern(
      const std::shared_ptr<ir::BinaryExpression>& binary) {
    if (!binary || binary->op != ir::BinaryOperator::Add) {
      return;
    }

    auto candidate_constant =
        std::dynamic_pointer_cast<ir::IntegerLiteral>(binary->right);
    if (!is_int32_constant(candidate_constant)) {
      return;
    }
    auto left_binary =
        std::dynamic_pointer_cast<ir::BinaryExpression>(binary->left);
    auto mul_by_scale = MulByScalePattern(left_binary);
    if (!mul_by_scale.matched) {
      return;
    }
    matched = true;
    expr = mul_by_scale.expr;
    scale = mul_by_scale.scale;
    constant = candidate_constant;
  }
};

/**
 * Matches the addition of two arbitrary expressions (e1 + e2).
 * Identifies the simple `Base + Index` calculation.
 */
struct AddExprExprPattern {
  /** True if the IR expression matches the target pattern. */
  bool matched = false;

  /** The first base expression (e1). */
  std::shared_ptr<ir::Expression> left = nullptr;

  /** The second base or index expression (e2). */
  std::shared_ptr<ir::Expression> right = nullptr;

  /**
   * Attempts to match the pattern against a binary expression.
   * @param binary - the IR binary expression to analyze.
   */
  explicit AddExprExprPattern(
      const std::shared_ptr<ir::BinaryExpression>& binary) {
    if (!binary || binary->op != ir::BinaryOperator::Add) {
      return;
    }
    matched = true;
    left = binary->left;
    right = binary->right;
  }
};

/**
 * Matches an expression added to a scaled expression (e1 + e2 * n).
 * Identifies the `Base + (Index * Scale)` instruction format.
 */
struct AddExprMulConstPattern {
  /** True if the IR expression matches the target pattern. */
  bool matched = false;

  /** The unscaled base expression (e1). */
  std::shared_ptr<ir::Expression> e1 = nullptr;

  /** The index expression being scaled (e2). */
  std::shared_ptr<ir::Expression> e2 = nullptr;

  /** The valid scale multiplier (n). */
  std::shared_ptr<ir::IntegerLiteral> n = nullptr;

  /**
   * Attempts to match the pattern against a binary expression.
   * @param binary - the IR binary expression to analyze.
   */
  explicit AddExprMulConstPattern(
      const std::shared_ptr<ir::BinaryExpression>& binary) {
    if (!binary || binary->op != ir::BinaryOperator::Add) {
      return;
    }
    auto right_binary =
        std::dynamic_pointer_cast<ir::BinaryExpression>(binary->right);
    auto mul_by_scale = MulByScalePattern(right_binary);
    if (!mul_by_scale.matched) {
      return;
    }
    matched = true;
    e1 = binary->left;
    e2 = mul_by_scale.expr;
    n = mul_by_scale.scale;
    matched = true;
  }
};

/**
 * Matches the full LEA addressing mode capabilities (e1 + e2 * n1 + n2).
 * Identifies the complete `Base + (Index * Scale) + Displacement` calculation.
 */
struct AddExprMulConstAddConstPattern {
  /** True if the IR expression matches the target pattern. */
  bool matched = false;

  /** The base expression (e1). */
  std::shared_ptr<ir::Expression> e1 = nullptr;

  /** The index expression (e2). */
  std::shared_ptr<ir::Expression> e2 = nullptr;

  /** The valid scale multiplier (n1). */
  std::shared_ptr<ir::IntegerLiteral> n1 = nullptr;

  /** The 32-bit integer constant displacement (n2). */
  std::shared_ptr<ir::IntegerLiteral> n2 = nullptr;

  /**
   * Attempts to match the full pattern using two different AST associativity passes.
   * @param binary - the IR binary expression to analyze.
   */
  explicit AddExprMulConstAddConstPattern(
      const std::shared_ptr<ir::BinaryExpression>& binary) {
    if (!binary || binary->op != ir::BinaryOperator::Add) {
      return;
    }
    // pass 1
    auto right_binary =
        std::dynamic_pointer_cast<ir::BinaryExpression>(binary->right);
    auto mul_scale_add_const = MulScaleAddConstPattern(right_binary);
    if (mul_scale_add_const.matched) {
      matched = true;
      e1 = binary->left;
      e2 = mul_scale_add_const.expr;
      n1 = mul_scale_add_const.scale;
      n2 = mul_scale_add_const.constant;
      return;
    }
    // pass 2
    auto candidate_constant =
        std::dynamic_pointer_cast<ir::IntegerLiteral>(binary->right);
    if (!is_int32_constant(candidate_constant)) {
      return;
    }
    auto left_binary =
        std::dynamic_pointer_cast<ir::BinaryExpression>(binary->left);
    auto add_mul_const = AddExprMulConstPattern(left_binary);
    if (!add_mul_const.matched) {
      return;
    }
    matched = true;
    e1 = add_mul_const.e1;
    e2 = add_mul_const.e2;
    n1 = add_mul_const.n;
    n2 = candidate_constant;
  }
};

std::any EtaIRtoASMVisitor::visit(ir::Statement& node) {
  return node.accept(*this);
}

std::any EtaIRtoASMVisitor::visit(ir::Assignment& node) {
  return std::visit(
      utils::overloaded{
          [node](my_compiler::ir::Temporary& t) {
            EtaASMTemporaryAllocatorVisitor helper(
                codegen::TemporaryOperand(t.name));
            return std::any_cast<codegen::CommandSequence>(
                helper.visit(*node.expression));
          },
          [node](my_compiler::ir::MemoryAccess& m) {
            auto binary_ptr =
                std::dynamic_pointer_cast<ir::BinaryExpression>(m.expr);
            auto add_expr_mul_const_add_const =
                AddExprMulConstAddConstPattern(binary_ptr);
            if (add_expr_mul_const_add_const.matched) {  // e1 + e2 * n1 +n2
              const auto t_1 = codegen::create_temporary("t_1");
              const auto t_2 = codegen::create_temporary("t_2");
              const auto t = codegen::create_temporary("t");
              EtaASMTemporaryAllocatorVisitor expr1(t_1);
              EtaASMTemporaryAllocatorVisitor expr2(t_2);
              EtaASMTemporaryAllocatorVisitor expr(t);
              auto sequence1 = std::any_cast<codegen::CommandSequence>(
                  expr1.visit(*add_expr_mul_const_add_const.e1));
              auto sequence2 = std::any_cast<codegen::CommandSequence>(
                  expr2.visit(*add_expr_mul_const_add_const.e2));
              auto sequence3 = std::any_cast<codegen::CommandSequence>(
                  expr.visit(*node.expression));
              sequence1.commands.insert(sequence1.commands.end(),
                                        sequence2.commands.begin(),
                                        sequence2.commands.end());
              sequence1.commands.insert(sequence1.commands.end(),
                                        sequence3.commands.begin(),
                                        sequence3.commands.end());
              sequence1.commands.emplace_back(
                  std::make_shared<codegen::Operation>(
                      codegen::OpCode::MOV,
                      std::make_shared<codegen::MemoryOperand>(
                          codegen::Size::QWORD_PTR, t_1, t_2,
                          static_cast<int>(
                              add_expr_mul_const_add_const.n1->value),
                          codegen::ConstantOperand(
                              add_expr_mul_const_add_const.n2->value)),
                      std::make_shared<codegen::TemporaryOperand>(t)));
              return sequence1;
            }
            auto add_expr_mul_const = AddExprMulConstPattern(binary_ptr);
            if (add_expr_mul_const.matched) {  // e1 + e2 * n
              const auto t_1 = codegen::create_temporary("t_1");
              const auto t_2 = codegen::create_temporary("t_2");
              const auto t = codegen::create_temporary("t");
              EtaASMTemporaryAllocatorVisitor expr1(t_1);
              EtaASMTemporaryAllocatorVisitor expr2(t_2);
              EtaASMTemporaryAllocatorVisitor expr(t);
              auto sequence1 = std::any_cast<codegen::CommandSequence>(
                  expr1.visit(*add_expr_mul_const.e1));
              auto sequence2 = std::any_cast<codegen::CommandSequence>(
                  expr2.visit(*add_expr_mul_const.e2));
              auto sequence3 = std::any_cast<codegen::CommandSequence>(
                  expr.visit(*node.expression));
              sequence1.commands.insert(sequence1.commands.end(),
                                        sequence2.commands.begin(),
                                        sequence2.commands.end());
              sequence1.commands.insert(sequence1.commands.end(),
                                        sequence3.commands.begin(),
                                        sequence3.commands.end());
              sequence1.commands.emplace_back(
                  std::make_shared<codegen::Operation>(
                      codegen::OpCode::MOV,
                      std::make_shared<codegen::MemoryOperand>(
                          codegen::Size::QWORD_PTR, t_1, t_2,
                          static_cast<int>(add_expr_mul_const.n->value)),
                      std::make_shared<codegen::TemporaryOperand>(t)));
              return sequence1;
            }
            const auto t_p = codegen::create_temporary("t_p");
            EtaASMTemporaryAllocatorVisitor helper(t_p);
            auto sequence = std::any_cast<codegen::CommandSequence>(
                helper.visit(*node.expression));
            auto memory_access_temp =
                std::dynamic_pointer_cast<ir::Temporary>(m.expr);
            if (memory_access_temp != nullptr) {
              sequence.commands.emplace_back(
                  std::make_shared<codegen::Operation>(
                      codegen::OpCode::MOV,
                      std::make_shared<codegen::MemoryOperand>(
                          codegen::Size::QWORD_PTR,
                          codegen::TemporaryOperand(memory_access_temp->name)),
                      std::make_shared<codegen::TemporaryOperand>(t_p)));
              return sequence;
            }
            auto memory_access_address =
                std::dynamic_pointer_cast<ir::Address>(m.expr);
            if (memory_access_address != nullptr) {
              sequence.commands.emplace_back(
                  std::make_shared<codegen::Operation>(
                      codegen::OpCode::MOV,
                      std::make_shared<codegen::MemoryOperand>(
                          codegen::Size::QWORD_PTR,
                          codegen::LabelOperand(memory_access_address->label)),
                      std::make_shared<codegen::TemporaryOperand>(t_p)));
              return sequence;
            }
            const auto t_2 = codegen::create_temporary(
                "t_2");  // temporary for storing memory access index
            EtaASMTemporaryAllocatorVisitor memory_access_helper(t_2);
            auto memory_access_sequence =
                std::any_cast<codegen::CommandSequence>(
                    memory_access_helper.visit(*m.expr));
            sequence.commands.insert(sequence.commands.end(),
                                     memory_access_sequence.commands.begin(),
                                     memory_access_sequence.commands.end());
            sequence.commands.emplace_back(std::make_shared<codegen::Operation>(
                codegen::OpCode::MOV,
                std::make_shared<codegen::MemoryOperand>(
                    codegen::Size::QWORD_PTR, codegen::TemporaryOperand(t_2)),
                std::make_shared<codegen::TemporaryOperand>(t_p)));
            return sequence;
          }},
      node.dest);
}

std::any EtaIRtoASMVisitor::visit(ir::StatementSequence& node) {
  std::vector<std::shared_ptr<codegen::Command>> commands;
  for (const auto& stmt_ptr : node.statements) {
    auto stmt_seq =
        std::any_cast<codegen::CommandSequence>(stmt_ptr->accept(*this));
    commands.insert(commands.end(), stmt_seq.commands.begin(),
                    stmt_seq.commands.end());
  }
  return codegen::CommandSequence(commands);
}

std::any EtaIRtoASMVisitor::visit(ir::Jump& node) {
  auto address_ptr = std::dynamic_pointer_cast<ir::Address>(node.target);
  if (address_ptr == nullptr) {
    throw std::runtime_error("Jump node expression must be an Address");
  }
  return codegen::CommandSequence({std::make_shared<codegen::Operation>(
      codegen::OpCode::JMP, std::make_shared<codegen::LabelOperand>(
                                std::format(".{}", address_ptr->label)))});
}

std::any EtaIRtoASMVisitor::visit(ir::Label& node) {
  return codegen::CommandSequence({std::make_shared<codegen::LabelDeclaration>(
      std::format(".{}", node.name))});
}

/**
 * Implements the jump_of(op) function used in the Codegen specification.
 * @param op - the IR opcode of the binary operator
 * @return the opcode of jump_of(op)
 */
inline codegen::OpCode jump_of(ir::BinaryOperator op) {
  switch (op) {
    case ir::BinaryOperator::Eq:
      return codegen::OpCode::JZ;
    case ir::BinaryOperator::Neq:
      return codegen::OpCode::JNZ;
    case ir::BinaryOperator::Lt:
      return codegen::OpCode::JL;
    case ir::BinaryOperator::Gt:
      return codegen::OpCode::JG;
    case ir::BinaryOperator::Ult:
      return codegen::OpCode::JB;
    case ir::BinaryOperator::Geq:
      return codegen::OpCode::JGE;
    case ir::BinaryOperator::Leq:
      return codegen::OpCode::JLE;
    default:
      throw std::runtime_error("Unrecognized operand.");
  }
}

/**
 * Implements the jump_of_test(op) function used in the Codegen specification
 *
 * @param op - the IR opcode of the binary operator
 * @return the opcode of jump_of_test(op)
 */
inline codegen::OpCode jump_of_test(ir::BinaryOperator op) {
  switch (op) {
    case ir::BinaryOperator::Eq:
      return codegen::OpCode::JZ;
    case ir::BinaryOperator::Neq:
      return codegen::OpCode::JNZ;
    case ir::BinaryOperator::Lt:
      return codegen::OpCode::JS;
    case ir::BinaryOperator::Geq:
      return codegen::OpCode::JNS;
    default:
      throw std::runtime_error("Unrecognized operand.");
  }
}

std::any EtaIRtoASMVisitor::visit(ir::ConditionalJump& node) {
  auto expr = std::dynamic_pointer_cast<ir::BinaryExpression>(node.condition);
  if (expr && (expr->op == ir::BinaryOperator::Eq ||
               expr->op == ir::BinaryOperator::Neq ||
               expr->op == ir::BinaryOperator::Lt ||
               expr->op == ir::BinaryOperator::Gt ||
               expr->op == ir::BinaryOperator::Ult ||
               expr->op == ir::BinaryOperator::Geq ||
               expr->op == ir::BinaryOperator::Leq)) {
    auto right_const =
        std::dynamic_pointer_cast<ir::IntegerLiteral>(expr->right);
    if ((expr->op == ir::BinaryOperator::Eq ||
         expr->op == ir::BinaryOperator::Neq ||
         expr->op == ir::BinaryOperator::Lt ||
         expr->op == ir::BinaryOperator::Geq) &&
        right_const && right_const->value == 0) {
      auto temp = codegen::TemporaryOperand("t");
      EtaASMTemporaryAllocatorVisitor helper(temp);
      auto sequence =
          std::any_cast<codegen::CommandSequence>(helper.visit(*expr->left));
      sequence.commands.emplace_back(std::make_shared<codegen::Operation>(
          codegen::OpCode::TEST,
          std::make_shared<codegen::TemporaryOperand>(temp),
          std::make_shared<codegen::TemporaryOperand>(temp)));
      sequence.commands.emplace_back(std::make_shared<codegen::Operation>(
          jump_of_test(expr->op), std::make_shared<codegen::LabelOperand>(
                                      std::format(".{}", node.ifLabelName))));
      return sequence;
    }
    auto t_1 = codegen::TemporaryOperand("t_1");
    auto t_2 = codegen::TemporaryOperand("t_2");
    EtaASMTemporaryAllocatorVisitor left(t_1);
    EtaASMTemporaryAllocatorVisitor right(t_2);
    auto sequence1 =
        std::any_cast<codegen::CommandSequence>(left.visit(*expr->left));
    auto sequence2 =
        std::any_cast<codegen::CommandSequence>(right.visit(*expr->right));
    sequence1.commands.insert(sequence1.commands.end(),
                              sequence2.commands.begin(),
                              sequence2.commands.end());
    sequence1.commands.emplace_back(std::make_shared<codegen::Operation>(
        codegen::OpCode::CMP, std::make_shared<codegen::TemporaryOperand>(t_1),
        std::make_shared<codegen::TemporaryOperand>(t_2)));
    sequence1.commands.emplace_back(std::make_shared<codegen::Operation>(
        jump_of(expr->op), std::make_shared<codegen::LabelOperand>(
                               std::format(".{}", node.ifLabelName))));
    return sequence1;
  }
  auto temp = codegen::TemporaryOperand("t");
  EtaASMTemporaryAllocatorVisitor helper(temp);
  auto sequence =
      std::any_cast<codegen::CommandSequence>(helper.visit(*node.condition));
  sequence.commands.emplace_back(std::make_shared<codegen::Operation>(
      codegen::OpCode::CMP, std::make_shared<codegen::TemporaryOperand>(temp),
      std::make_shared<codegen::ConstantOperand>(0)));
  sequence.commands.emplace_back(std::make_shared<codegen::Operation>(
      codegen::OpCode::JNE, std::make_shared<codegen::LabelOperand>(
                                std::format(".{}", node.ifLabelName))));
  return sequence;
}

/**
 * Insert padding on the stack if number of arguments is even.
 *
 * @param k - number of arguments for the function.
 * @return the padding operation (if needed).
 */
std::shared_ptr<codegen::Operation> insert_padding(std::size_t k) {
  if (k % 2 == 0) {
    return std::make_shared<codegen::Operation>(
        codegen::OpCode::SUB,
        std::make_shared<codegen::RegisterOperand>(codegen::Register::RSP),
        std::make_shared<codegen::ConstantOperand>(8));
  } else
    return nullptr;
}

/**
 * Remove padding on the stack if number of arguments is even.
 *
 * @param k - number of arguments for the function.
 * @return the padding operation (if needed).
 */
std::shared_ptr<codegen::Operation> remove_padding(std::size_t k) {
  if (k % 2 == 0) {
    return std::make_shared<codegen::Operation>(
        codegen::OpCode::ADD,
        std::make_shared<codegen::RegisterOperand>(codegen::Register::RSP),
        std::make_shared<codegen::ConstantOperand>(8));
  } else
    return nullptr;
}

std::any EtaIRtoASMVisitor::visit(ir::CallStatement& node) {
  std::vector<std::shared_ptr<codegen::Command>> result{};
  std::size_t count = 1;
  std::vector<codegen::TemporaryOperand> temp_list{};
  std::string name =
      std::dynamic_pointer_cast<ir::Address>(node.function)->label;
  for (auto& e : node.arguments) {
    codegen::TemporaryOperand t(
        codegen::create_temporary(std::format("t_{}", count)));
    temp_list.push_back(t);
    auto visitor = EtaASMTemporaryAllocatorVisitor(t);
    auto cmd_output =
        std::any_cast<codegen::CommandSequence>(e->accept(visitor));
    result.insert(result.end(), cmd_output.commands.begin(),
                  cmd_output.commands.end());
    count++;
  }
  if (node.numReturns <= 2) {
    const std::size_t stack_arg_count =
        node.arguments.size() > 6 ? node.arguments.size() - 6 : 0;
    if (auto pad = insert_padding(stack_arg_count + 1)) {
      result.push_back(pad);
    }
    for (std::size_t i = node.arguments.size(); i >= 1; --i) {
      if (i >= 7) {
        result.push_back(std::make_shared<codegen::Operation>(
            codegen::OpCode::PUSH, std::make_shared<codegen::TemporaryOperand>(
                                       std::move(temp_list[i - 1]))));
      } else {
        result.push_back(std::make_shared<codegen::Operation>(
            codegen::OpCode::MOV, loc_of_arg(i),
            std::make_shared<codegen::TemporaryOperand>(
                std::move(temp_list[i - 1]))));
      }
    }
    result.push_back(std::make_shared<codegen::Operation>(
        codegen::OpCode::CALL, std::make_shared<codegen::LabelOperand>(name),
        node.arguments.size()));
    for (std::size_t i = 1; i <= node.numReturns; ++i) {
      result.push_back(std::make_shared<codegen::Operation>(
          codegen::OpCode::MOV,
          std::make_shared<codegen::TemporaryOperand>(std::format("_RV{}", i)),
          loc_of_rv(i)));
    }
    result.push_back(std::make_shared<codegen::Operation>(
        codegen::OpCode::ADD,
        std::make_shared<codegen::RegisterOperand>(codegen::Register::RSP),
        std::make_shared<codegen::ConstantOperand>(
            static_cast<int64_t>(8 * stack_arg_count))));
    if (auto pad = remove_padding(stack_arg_count + 1)) {
      result.push_back(pad);
    }
  } else {
    const std::size_t stack_arg_count =
        node.arguments.size() > 5 ? node.arguments.size() - 5 : 0;
    if (auto pad = insert_padding(stack_arg_count + node.numReturns - 2 + 1)) {
      result.push_back(pad);
    }
    result.push_back(std::make_shared<codegen::Operation>(
        codegen::OpCode::SUB,
        std::make_shared<codegen::RegisterOperand>(codegen::Register::RSP),
        std::make_shared<codegen::ConstantOperand>(
            static_cast<int64_t>(8 * (node.numReturns - 2)))));
    result.push_back(std::make_shared<codegen::Operation>(
        codegen::OpCode::MOV, loc_of_arg(1),
        std::make_shared<codegen::RegisterOperand>(codegen::Register::RSP)));
    for (std::size_t i = node.arguments.size(); i >= 1; --i) {
      if (i >= 6) {
        result.push_back(std::make_shared<codegen::Operation>(
            codegen::OpCode::PUSH, std::make_shared<codegen::TemporaryOperand>(
                                       std::move(temp_list[i - 1]))));
      } else {
        result.push_back(std::make_shared<codegen::Operation>(
            codegen::OpCode::MOV, loc_of_arg(i + 1),
            std::make_shared<codegen::TemporaryOperand>(
                std::move(temp_list[i - 1]))));
      }
    }
    result.push_back(std::make_shared<codegen::Operation>(
        codegen::OpCode::CALL, std::make_shared<codegen::LabelOperand>(name),
        node.arguments.size() + 1)); // add 1 for _RVLOC
    result.push_back(std::make_shared<codegen::Operation>(
        codegen::OpCode::ADD,
        std::make_shared<codegen::RegisterOperand>(codegen::Register::RSP),
        std::make_shared<codegen::ConstantOperand>(
            static_cast<int64_t>(8 * stack_arg_count))));
    for (std::size_t i = 1; i <= 2; ++i) {
      result.push_back(std::make_shared<codegen::Operation>(
          codegen::OpCode::MOV,
          std::make_shared<codegen::TemporaryOperand>(std::format("_RV{}", i)),
          loc_of_rv(i)));
    }
    for (std::size_t i = 3; i <= node.numReturns; ++i) {
      result.push_back(std::make_shared<codegen::Operation>(
          codegen::OpCode::POP, std::make_shared<codegen::TemporaryOperand>(
                                    std::format("_RV{}", i))));
    }
    if (auto pad = remove_padding(stack_arg_count + node.numReturns - 2 + 1)) {
      result.push_back(pad);
    }
  }
  return codegen::CommandSequence(result);
}

std::any EtaIRtoASMVisitor::visit(ir::ReturnStatement& node) {
  std::vector<std::shared_ptr<codegen::Command>> result{};
  std::size_t count = 1;
  std::vector<codegen::TemporaryOperand> temp_list{};
  for (auto& e : node.results) {
    codegen::TemporaryOperand t(
        codegen::create_temporary(std::format("t_{}", count)));
    temp_list.push_back(t);
    auto visitor = EtaASMTemporaryAllocatorVisitor(t);
    auto cmd_output =
        std::any_cast<codegen::CommandSequence>(e->accept(visitor));
    result.insert(result.end(), cmd_output.commands.begin(),
                  cmd_output.commands.end());
    count++;
  }
  for (std::size_t i = 1; i <= node.results.size(); i++) {
    result.push_back(std::make_shared<codegen::Operation>(
        codegen::OpCode::MOV, loc_of_rv(i),
        std::make_shared<codegen::TemporaryOperand>(
            std::move(temp_list[i - 1]))));
  }

  // restore callee-save registers
  for (const auto& [temp, reg] : temps_to_csr) {
    result.push_back(std::make_shared<codegen::Operation>(codegen::OpCode::MOV,
    std::make_shared<codegen::RegisterOperand>(reg),
      std::make_shared<codegen::TemporaryOperand>(temp)
      ));
  }

  result.push_back(
      std::make_shared<codegen::Operation>(codegen::OpCode::LEAVE));
  result.push_back(std::make_shared<codegen::Operation>(codegen::OpCode::RET, node.results.size()));
  return codegen::CommandSequence(result);
}

// EtaASMTemporaryAllocatorVisitor
std::any EtaASMTemporaryAllocatorVisitor::visit(ir::Expression& node) {
  return node.accept(*this);
}

std::any EtaASMTemporaryAllocatorVisitor::visit(ir::IntegerLiteral& node) {
  if (node.value == 0) {
    return codegen::CommandSequence{
        std::vector<std::shared_ptr<codegen::Command>>{
            std::make_shared<codegen::Operation>(
                codegen::OpCode::XOR,
                std::make_shared<codegen::TemporaryOperand>(temp),
                std::make_shared<codegen::TemporaryOperand>(temp))}};
  } else {
    return codegen::CommandSequence{
        std::vector<std::shared_ptr<codegen::Command>>{
            std::make_shared<codegen::Operation>(
                codegen::OpCode::MOV,
                std::make_shared<codegen::TemporaryOperand>(temp),
                std::make_shared<codegen::ConstantOperand>(node.value))}};
  }
}

std::any EtaASMTemporaryAllocatorVisitor::visit(ir::Temporary& node) {
  if (temp.name == node.name)
    return codegen::CommandSequence{
        std::vector<std::shared_ptr<codegen::Command>>{}};
  return codegen::CommandSequence{
      std::vector<std::shared_ptr<codegen::Command>>{
          std::make_shared<codegen::Operation>(
              codegen::OpCode::MOV,
              std::make_shared<codegen::TemporaryOperand>(temp),
              std::make_shared<codegen::TemporaryOperand>(node.name))}};
}

// Helper: Return the asm of an operator
inline codegen::OpCode asm_of(const ir::BinaryOperator op) {
  switch (op) {
    case ir::BinaryOperator::Add:
      return codegen::OpCode::ADD;
    case ir::BinaryOperator::Sub:
      return codegen::OpCode::SUB;
    case ir::BinaryOperator::Mul:
      return codegen::OpCode::IMUL;
    case ir::BinaryOperator::And:
      return codegen::OpCode::AND;
    case ir::BinaryOperator::Or:
      return codegen::OpCode::OR;
    case ir::BinaryOperator::Xor:
      return codegen::OpCode::XOR;
    case ir::BinaryOperator::Eq:
      return codegen::OpCode::SETE;
    case ir::BinaryOperator::Neq:
      return codegen::OpCode::SETNE;
    case ir::BinaryOperator::Lt:
      return codegen::OpCode::SETL;
    case ir::BinaryOperator::Gt:
      return codegen::OpCode::SETG;
    case ir::BinaryOperator::Ult:
      return codegen::OpCode::SETB;
    case ir::BinaryOperator::Geq:
      return codegen::OpCode::SETGE;
    case ir::BinaryOperator::Leq:
      return codegen::OpCode::SETLE;
    case ir::BinaryOperator::Rshift:
      return codegen::OpCode::SHR;
    case ir::BinaryOperator::Lshift:
      return codegen::OpCode::SHL;
    case ir::BinaryOperator::Arshift:
      return codegen::OpCode::SAR;
    default:
      throw std::runtime_error(
          std::format("Asm of ir operand {} is not implemented.",
                      BinaryOperator2String(op)));
  }
}

/**
 * Implements the compute(t, e1, op, e2) function used in the codegen specification.
 *
 * @param target t in the formal specification
 * @param left e1 in the formal specification
 * @param right e2 in the formal specification
 * @param op op in the formal specification
 * @return the sequence of abstract ASM instructions generated by compute(t, e1,
 * op, e2)
 */
codegen::CommandSequence compute(const codegen::TemporaryOperand& target,
                                 const codegen::TemporaryOperand& left,
                                 const codegen::TemporaryOperand& right,
                                 const ir::BinaryOperator op) {
  if (op == ir::BinaryOperator::Add || op == ir::BinaryOperator::Sub ||
      op == ir::BinaryOperator::Mul || op == ir::BinaryOperator::And ||
      op == ir::BinaryOperator::Or || op == ir::BinaryOperator::Xor) {
    return codegen::CommandSequence(
        std::vector<std::shared_ptr<codegen::Command>>{
            std::make_shared<codegen::Operation>(
                asm_of(op), std::make_shared<codegen::TemporaryOperand>(left),
                std::make_shared<codegen::TemporaryOperand>(right)),
            std::make_shared<codegen::Operation>(
                codegen::OpCode::MOV,
                std::make_shared<codegen::TemporaryOperand>(target),
                std::make_shared<codegen::TemporaryOperand>(left))});
  } else if (op == ir::BinaryOperator::Eq || op == ir::BinaryOperator::Neq ||
             op == ir::BinaryOperator::Gt || op == ir::BinaryOperator::Geq ||
             op == ir::BinaryOperator::Lt || op == ir::BinaryOperator::Leq ||
             op == ir::BinaryOperator::Ult) {
    return codegen::CommandSequence(
        std::vector<std::shared_ptr<codegen::Command>>{
            std::make_shared<codegen::Operation>(
                codegen::OpCode::XOR,
                std::make_shared<codegen::RegisterOperand>(
                    codegen::Register::RAX),
                std::make_shared<codegen::RegisterOperand>(
                    codegen::Register::RAX)),
            std::make_shared<codegen::Operation>(
                codegen::OpCode::CMP,
                std::make_shared<codegen::TemporaryOperand>(left),
                std::make_shared<codegen::TemporaryOperand>(right)),
            std::make_shared<codegen::Operation>(
                asm_of(op), std::make_shared<codegen::RegisterOperand>(
                                codegen::Register::AL)),
            std::make_shared<codegen::Operation>(
                codegen::OpCode::MOV,
                std::make_shared<codegen::TemporaryOperand>(target),
                std::make_shared<codegen::RegisterOperand>(
                    codegen::Register::RAX))});
  } else if (op == ir::BinaryOperator::Div) {
    return codegen::CommandSequence(
        std::vector<std::shared_ptr<codegen::Command>>{
            std::make_shared<codegen::Operation>(
                codegen::OpCode::MOV,
                std::make_shared<codegen::RegisterOperand>(
                    codegen::Register::RAX),
                std::make_shared<codegen::TemporaryOperand>(left)),
            std::make_shared<codegen::Operation>(codegen::OpCode::CQO),
            std::make_shared<codegen::Operation>(
                codegen::OpCode::IDIV,
                std::make_shared<codegen::TemporaryOperand>(right)),
            std::make_shared<codegen::Operation>(
                codegen::OpCode::MOV,
                std::make_shared<codegen::TemporaryOperand>(target),
                std::make_shared<codegen::RegisterOperand>(
                    codegen::Register::RAX))});
  } else if (op == ir::BinaryOperator::Mod) {
    return codegen::CommandSequence(
        std::vector<std::shared_ptr<codegen::Command>>{
            std::make_shared<codegen::Operation>(
                codegen::OpCode::MOV,
                std::make_shared<codegen::RegisterOperand>(
                    codegen::Register::RAX),
                std::make_shared<codegen::TemporaryOperand>(left)),
            std::make_shared<codegen::Operation>(codegen::OpCode::CQO),
            std::make_shared<codegen::Operation>(
                codegen::OpCode::IDIV,
                std::make_shared<codegen::TemporaryOperand>(right)),
            std::make_shared<codegen::Operation>(
                codegen::OpCode::MOV,
                std::make_shared<codegen::TemporaryOperand>(target),
                std::make_shared<codegen::RegisterOperand>(
                    codegen::Register::RDX))});
  } else if (op == ir::BinaryOperator::Hmul) {
    return codegen::CommandSequence(
        std::vector<std::shared_ptr<codegen::Command>>{
            std::make_shared<codegen::Operation>(
                codegen::OpCode::MOV,
                std::make_shared<codegen::RegisterOperand>(
                    codegen::Register::RAX),
                std::make_shared<codegen::TemporaryOperand>(left)),
            std::make_shared<codegen::Operation>(
                codegen::OpCode::IMUL,
                std::make_shared<codegen::TemporaryOperand>(right)),
            std::make_shared<codegen::Operation>(
                codegen::OpCode::MOV,
                std::make_shared<codegen::TemporaryOperand>(target),
                std::make_shared<codegen::RegisterOperand>(
                    codegen::Register::RDX))});
  } else if (op == ir::BinaryOperator::Rshift ||
             op == ir::BinaryOperator::Lshift ||
             op == ir::BinaryOperator::Arshift) {
    return codegen::CommandSequence(
        std::vector<std::shared_ptr<codegen::Command>>{
            std::make_shared<codegen::Operation>(
                codegen::OpCode::MOV,
                std::make_shared<codegen::RegisterOperand>(
                    codegen::Register::RCX),
                std::make_shared<codegen::TemporaryOperand>(right)),
            std::make_shared<codegen::Operation>(
                asm_of(op), std::make_shared<codegen::TemporaryOperand>(left),
                std::make_shared<codegen::RegisterOperand>(
                    codegen::Register::CL)),
            std::make_shared<codegen::Operation>(
                codegen::OpCode::MOV,
                std::make_shared<codegen::TemporaryOperand>(target),
                std::make_shared<codegen::TemporaryOperand>(left))});
  }
  throw std::runtime_error("Unexpected ir operand");
}

std::any EtaASMTemporaryAllocatorVisitor::visit(ir::BinaryExpression& node) {
  const auto binary_ptr = std::shared_ptr<ir::BinaryExpression>(
      &node, [](ir::BinaryExpression*) {});

  if (node.op == ir::BinaryOperator::Sub) {
    auto left_const = std::dynamic_pointer_cast<ir::IntegerLiteral>(node.left);
    if (left_const && left_const->value == 0) {  // 0 sub e
      auto t_1 = codegen::create_temporary("t_1");
      EtaASMTemporaryAllocatorVisitor right(t_1);
      auto operation1 =
          std::any_cast<codegen::CommandSequence>(right.visit(*node.right));
      operation1.commands.push_back(std::make_shared<codegen::Operation>(
          codegen::OpCode::NEG,
          std::make_shared<codegen::TemporaryOperand>(t_1)));
      operation1.commands.push_back(std::make_shared<codegen::Operation>(
          codegen::OpCode::MOV,
          std::make_shared<codegen::TemporaryOperand>(temp),
          std::make_shared<codegen::TemporaryOperand>(t_1)));
      return operation1;
    }
  }
  if (node.op == ir::BinaryOperator::Mul) {
    auto mul_by_scale = MulByScalePattern(binary_ptr);
    auto right_const =
        std::dynamic_pointer_cast<ir::IntegerLiteral>(node.right);
    if (mul_by_scale.matched) {  // e mul n
      auto t_1 = codegen::create_temporary("t_1");
      EtaASMTemporaryAllocatorVisitor expr(t_1);
      auto operation1 = std::any_cast<codegen::CommandSequence>(
          expr.visit(*mul_by_scale.expr));
      operation1.commands.push_back(std::make_shared<codegen::Operation>(
          codegen::OpCode::LEA,
          std::make_shared<codegen::TemporaryOperand>(temp),
          std::make_shared<codegen::MemoryOperand>(codegen::Size::QWORD_PTR,
                                                   std::nullopt, t_1,
                                                   mul_by_scale.scale->value)));
      return operation1;
    }
    if (right_const) {
      // if a number has a single bit, it is a power of 2
      if (right_const->value > 1 && std::has_single_bit(static_cast<uint64_t>(
                                        right_const->value))) {  // e mul 2^n
        auto t_1 = codegen::create_temporary("t_1");
        EtaASMTemporaryAllocatorVisitor left(t_1);
        auto operation1 =
            std::any_cast<codegen::CommandSequence>(left.visit(*node.left));
        operation1.commands.push_back(std::make_shared<codegen::Operation>(
            codegen::OpCode::SHL,
            std::make_shared<codegen::TemporaryOperand>(t_1),
            std::make_shared<codegen::ConstantOperand>(
                std::bit_width(static_cast<uint64_t>(right_const->value)) -
                1  // log2(right_const->value)
                )));
        operation1.commands.push_back(std::make_shared<codegen::Operation>(
            codegen::OpCode::MOV,
            std::make_shared<codegen::TemporaryOperand>(temp),
            std::make_shared<codegen::TemporaryOperand>(t_1)));
        return operation1;
      }
    }
  }
  if (node.op == ir::BinaryOperator::Div) {
    auto right_const =
        std::dynamic_pointer_cast<ir::IntegerLiteral>(node.right);
    if (right_const && right_const->value > 1 &&
        std::has_single_bit(
            static_cast<uint64_t>(right_const->value))) {  // e div 2^n
      auto t_1 = codegen::create_temporary("t_1");
      EtaASMTemporaryAllocatorVisitor left(t_1);
      auto operation1 =
          std::any_cast<codegen::CommandSequence>(left.visit(*node.left));
      operation1.commands.push_back(std::make_shared<codegen::Operation>(
          codegen::OpCode::SAR,
          std::make_shared<codegen::TemporaryOperand>(t_1),
          std::make_shared<codegen::ConstantOperand>(
              std::bit_width(static_cast<uint64_t>(right_const->value)) -
              1  // log2(right_const->value)
              )));
      operation1.commands.push_back(std::make_shared<codegen::Operation>(
          codegen::OpCode::MOV,
          std::make_shared<codegen::TemporaryOperand>(temp),
          std::make_shared<codegen::TemporaryOperand>(t_1)));
      return operation1;
    }
  }
  if (node.op == ir::BinaryOperator::Add) {
    auto add_expr_mul_const_add_const =
        AddExprMulConstAddConstPattern(binary_ptr);
    if (add_expr_mul_const_add_const.matched) {  // e1 + e2 * n1 +n2
      auto t_1 = codegen::create_temporary("t_1");
      EtaASMTemporaryAllocatorVisitor expr1(t_1);
      auto t_2 = codegen::create_temporary("t_2");
      EtaASMTemporaryAllocatorVisitor expr2(t_2);
      auto operation1 = std::any_cast<codegen::CommandSequence>(
          expr1.visit(*add_expr_mul_const_add_const.e1));
      auto operation2 = std::any_cast<codegen::CommandSequence>(
          expr2.visit(*add_expr_mul_const_add_const.e2));
      operation1.commands.insert(operation1.commands.end(),
                                 operation2.commands.begin(),
                                 operation2.commands.end());
      operation1.commands.push_back(std::make_shared<codegen::Operation>(
          codegen::OpCode::LEA,
          std::make_shared<codegen::TemporaryOperand>(temp),
          std::make_shared<codegen::MemoryOperand>(
              codegen::Size::QWORD_PTR, t_1, t_2,
              static_cast<int>(add_expr_mul_const_add_const.n1->value),
              codegen::ConstantOperand(
                  add_expr_mul_const_add_const.n2->value))));
      return operation1;
    }
    auto add_mul_const = AddExprMulConstPattern(binary_ptr);
    if (add_mul_const.matched) {  // e1 + e2 * n1
      auto t_1 = codegen::create_temporary("t_1");
      EtaASMTemporaryAllocatorVisitor expr1(t_1);
      auto t_2 = codegen::create_temporary("t_2");
      EtaASMTemporaryAllocatorVisitor expr2(t_2);
      auto operation1 = std::any_cast<codegen::CommandSequence>(
          expr1.visit(*add_mul_const.e1));
      auto operation2 = std::any_cast<codegen::CommandSequence>(
          expr2.visit(*add_mul_const.e2));
      operation1.commands.insert(operation1.commands.end(),
                                 operation2.commands.begin(),
                                 operation2.commands.end());
      operation1.commands.push_back(std::make_shared<codegen::Operation>(
          codegen::OpCode::LEA,
          std::make_shared<codegen::TemporaryOperand>(temp),
          std::make_shared<codegen::MemoryOperand>(
              codegen::Size::QWORD_PTR, t_1, t_2,
              static_cast<int>(add_mul_const.n->value))));
      return operation1;
    }
    auto mul_scale_add_const = MulScaleAddConstPattern(binary_ptr);
    if (mul_scale_add_const.matched) {  // e * n1 + n2
      auto t_1 = codegen::create_temporary("t_1");
      EtaASMTemporaryAllocatorVisitor expr(t_1);
      auto operation1 = std::any_cast<codegen::CommandSequence>(
          expr.visit(*mul_scale_add_const.expr));
      operation1.commands.push_back(std::make_shared<codegen::Operation>(
          codegen::OpCode::LEA,
          std::make_shared<codegen::TemporaryOperand>(temp),
          std::make_shared<codegen::MemoryOperand>(
              codegen::Size::QWORD_PTR, std::nullopt, t_1,
              mul_scale_add_const.scale->value,
              mul_scale_add_const.constant->value)));
      return operation1;
    }
    auto add_const = AddConstPattern(binary_ptr);
    if (add_const.matched) {  // e + n
      auto t_1 = codegen::create_temporary("t_1");
      EtaASMTemporaryAllocatorVisitor expr(t_1);
      auto operation1 =
          std::any_cast<codegen::CommandSequence>(expr.visit(*add_const.expr));
      operation1.commands.push_back(std::make_shared<codegen::Operation>(
          codegen::OpCode::LEA,
          std::make_shared<codegen::TemporaryOperand>(temp),
          std::make_shared<codegen::MemoryOperand>(
              codegen::Size::QWORD_PTR, t_1, std::nullopt, std::nullopt,
              add_const.constant->value)));
      return operation1;
    }
    // e1 + e2
    auto add_expr_expr = AddExprExprPattern(binary_ptr);
    auto t_1 = codegen::create_temporary("t_1");
    auto t_2 = codegen::create_temporary("t_2");
    EtaASMTemporaryAllocatorVisitor expr1(t_1);
    EtaASMTemporaryAllocatorVisitor expr2(t_2);
    auto operation1 = std::any_cast<codegen::CommandSequence>(
        expr1.visit(*add_expr_expr.left));
    auto operation2 = std::any_cast<codegen::CommandSequence>(
        expr2.visit(*add_expr_expr.right));
    operation1.commands.insert(operation1.commands.end(),
                               operation2.commands.begin(),
                               operation2.commands.end());
    operation1.commands.push_back(std::make_shared<codegen::Operation>(
        codegen::OpCode::LEA, std::make_shared<codegen::TemporaryOperand>(temp),
        std::make_shared<codegen::MemoryOperand>(codegen::Size::QWORD_PTR, t_1,
                                                 t_2)));
    return operation1;
  }
  auto t_1 = codegen::create_temporary("t_1");
  EtaASMTemporaryAllocatorVisitor left(t_1);
  auto t_2 = codegen::create_temporary("t_2");
  EtaASMTemporaryAllocatorVisitor right(t_2);
  auto operation1 =
      std::any_cast<codegen::CommandSequence>(left.visit(*node.left));
  auto operation2 =
      std::any_cast<codegen::CommandSequence>(right.visit(*node.right));
  auto operation3 = compute(temp, t_1, t_2, node.op);
  operation1.commands.insert(operation1.commands.end(),
                             operation2.commands.begin(),
                             operation2.commands.end());
  operation1.commands.insert(operation1.commands.end(),
                             operation3.commands.begin(),
                             operation3.commands.end());
  return operation1;
}

std::any EtaASMTemporaryAllocatorVisitor::visit(ir::MemoryAccess& node) {
  auto binary_ptr = std::dynamic_pointer_cast<ir::BinaryExpression>(node.expr);
  if (binary_ptr) {
    auto mul_by_scale = MulByScalePattern(binary_ptr);
    if (mul_by_scale.matched) {  // e * n
      auto t_1 = codegen::create_temporary("t_1");
      EtaASMTemporaryAllocatorVisitor expr(t_1);
      auto operation1 = std::any_cast<codegen::CommandSequence>(
          expr.visit(*mul_by_scale.expr));
      operation1.commands.push_back(std::make_shared<codegen::Operation>(
          codegen::OpCode::MOV,
          std::make_shared<codegen::TemporaryOperand>(temp),
          std::make_shared<codegen::MemoryOperand>(codegen::Size::QWORD_PTR,
                                                   std::nullopt, t_1,
                                                   mul_by_scale.scale->value)));
      return operation1;
    }
    auto add_expr_mul_const_add_const =
        AddExprMulConstAddConstPattern(binary_ptr);
    if (add_expr_mul_const_add_const.matched) {  // e1 + e2 * n1 +n2
      auto t_1 = codegen::create_temporary("t_1");
      EtaASMTemporaryAllocatorVisitor expr1(t_1);
      auto t_2 = codegen::create_temporary("t_2");
      EtaASMTemporaryAllocatorVisitor expr2(t_2);
      auto operation1 = std::any_cast<codegen::CommandSequence>(
          expr1.visit(*add_expr_mul_const_add_const.e1));
      auto operation2 = std::any_cast<codegen::CommandSequence>(
          expr2.visit(*add_expr_mul_const_add_const.e2));
      operation1.commands.insert(operation1.commands.end(),
                                 operation2.commands.begin(),
                                 operation2.commands.end());
      operation1.commands.push_back(std::make_shared<codegen::Operation>(
          codegen::OpCode::MOV,
          std::make_shared<codegen::TemporaryOperand>(temp),
          std::make_shared<codegen::MemoryOperand>(
              codegen::Size::QWORD_PTR, t_1, t_2,
              static_cast<int>(add_expr_mul_const_add_const.n1->value),
              codegen::ConstantOperand(
                  add_expr_mul_const_add_const.n2->value))));
      return operation1;
    }
    auto add_mul_const = AddExprMulConstPattern(binary_ptr);
    if (add_mul_const.matched) {  // e1 + e2 * n1
      auto t_1 = codegen::create_temporary("t_1");
      EtaASMTemporaryAllocatorVisitor expr1(t_1);
      auto t_2 = codegen::create_temporary("t_2");
      EtaASMTemporaryAllocatorVisitor expr2(t_2);
      auto operation1 = std::any_cast<codegen::CommandSequence>(
          expr1.visit(*add_mul_const.e1));
      auto operation2 = std::any_cast<codegen::CommandSequence>(
          expr2.visit(*add_mul_const.e2));
      operation1.commands.insert(operation1.commands.end(),
                                 operation2.commands.begin(),
                                 operation2.commands.end());
      operation1.commands.push_back(std::make_shared<codegen::Operation>(
          codegen::OpCode::MOV,
          std::make_shared<codegen::TemporaryOperand>(temp),
          std::make_shared<codegen::MemoryOperand>(
              codegen::Size::QWORD_PTR, t_1, t_2,
              static_cast<int>(add_mul_const.n->value))));
      return operation1;
    }
    auto mul_scale_add_const = MulScaleAddConstPattern(binary_ptr);
    if (mul_scale_add_const.matched) {  // e * n1 + n2
      auto t_1 = codegen::create_temporary("t_1");
      EtaASMTemporaryAllocatorVisitor expr(t_1);
      auto operation1 = std::any_cast<codegen::CommandSequence>(
          expr.visit(*mul_scale_add_const.expr));
      operation1.commands.push_back(std::make_shared<codegen::Operation>(
          codegen::OpCode::MOV,
          std::make_shared<codegen::TemporaryOperand>(temp),
          std::make_shared<codegen::MemoryOperand>(
              codegen::Size::QWORD_PTR, std::nullopt, t_1,
              mul_scale_add_const.scale->value,
              mul_scale_add_const.constant->value)));
      return operation1;
    }
    auto add_const = AddConstPattern(binary_ptr);
    if (add_const.matched) {  // e + n
      auto t_1 = codegen::create_temporary("t_1");
      EtaASMTemporaryAllocatorVisitor expr(t_1);
      auto operation1 =
          std::any_cast<codegen::CommandSequence>(expr.visit(*add_const.expr));
      operation1.commands.push_back(std::make_shared<codegen::Operation>(
          codegen::OpCode::MOV,
          std::make_shared<codegen::TemporaryOperand>(temp),
          std::make_shared<codegen::MemoryOperand>(
              codegen::Size::QWORD_PTR, t_1, std::nullopt, std::nullopt,
              add_const.constant->value)));
      return operation1;
    }
    auto add_expr_expr = AddExprExprPattern(binary_ptr);
    if (add_expr_expr.matched) {  // e1 + e2
      auto t_1 = codegen::create_temporary("t_1");
      auto t_2 = codegen::create_temporary("t_2");
      EtaASMTemporaryAllocatorVisitor expr1(t_1);
      EtaASMTemporaryAllocatorVisitor expr2(t_2);
      auto operation1 = std::any_cast<codegen::CommandSequence>(
          expr1.visit(*add_expr_expr.left));
      auto operation2 = std::any_cast<codegen::CommandSequence>(
          expr2.visit(*add_expr_expr.right));
      operation1.commands.insert(operation1.commands.end(),
                                 operation2.commands.begin(),
                                 operation2.commands.end());
      operation1.commands.push_back(std::make_shared<codegen::Operation>(
          codegen::OpCode::MOV,
          std::make_shared<codegen::TemporaryOperand>(temp),
          std::make_shared<codegen::MemoryOperand>(codegen::Size::QWORD_PTR,
                                                   t_1, t_2, 1)));
      return operation1;
    }
  }
  auto memory_access_address =
      std::dynamic_pointer_cast<ir::Address>(node.expr);
  if (memory_access_address != nullptr) {
    return codegen::CommandSequence({std::make_shared<codegen::Operation>(
        codegen::OpCode::MOV, std::make_shared<codegen::TemporaryOperand>(temp),
        std::make_shared<codegen::MemoryOperand>(
            codegen::Size::QWORD_PTR,
            codegen::LabelOperand(memory_access_address->label)))});
  }
  auto t_1 = codegen::create_temporary("t_1");
  EtaASMTemporaryAllocatorVisitor expr(t_1);
  auto operation1 =
      std::any_cast<codegen::CommandSequence>(expr.visit(*node.expr));
  operation1.commands.push_back(std::make_shared<codegen::Operation>(
      codegen::OpCode::MOV, std::make_shared<codegen::TemporaryOperand>(temp),
      std::make_shared<codegen::MemoryOperand>(codegen::Size::QWORD_PTR, t_1)));
  return operation1;
}

std::any EtaASMTemporaryAllocatorVisitor::visit(ir::Address& node) {
  auto operation1 =
      codegen::CommandSequence({std::make_shared<codegen::Operation>(
          codegen::OpCode::LEA,
          std::make_shared<codegen::TemporaryOperand>(temp),
          std::make_shared<codegen::MemoryOperand>(
              codegen::Size::QWORD_PTR, codegen::LabelOperand(node.label)))});
  return operation1;
}

}  // namespace ssy38::my_compiler::visitor
