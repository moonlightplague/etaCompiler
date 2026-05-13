#include "EtaConstantFolder.hpp"

#include "ir.hpp"

namespace ssy38::my_compiler::visitor {

template <typename T>
std::vector<T> visit_list(EtaConstantFolder &visitor, std::vector<T> lst) {
  std::vector<T> visited_lst;
  visited_lst.reserve(lst.size());
  for (auto &element : lst) {
    visited_lst.push_back(std::any_cast<T>(element.accept(visitor)));
  }
  return visited_lst;
}

template <typename T>
std::vector<std::shared_ptr<T>> visit_shared_list(
    EtaConstantFolder &visitor, std::vector<std::shared_ptr<T>> lst) {
  std::vector<std::shared_ptr<T>> visited_lst;
  visited_lst.reserve(lst.size());
  for (auto &element : lst) {
    visited_lst.push_back(
        std::any_cast<std::shared_ptr<T>>(element->accept(visitor)));
  }
  return visited_lst;
}

// the expression inside a data declaration is always constant
std::any EtaConstantFolder::visit(ir::DataDeclaration &node) {
  return ir::DataDeclaration(node);
}

// for a function declaration, we should fold all expressions inside the body
std::any EtaConstantFolder::visit(ir::FunctionDeclaration &node) {
  return ir::FunctionDeclaration(
      node.name,
      std::any_cast<std::shared_ptr<ir::Statement>>(node.body->accept(*this)),
      node.arg_count,
      node.return_val_count);
}

// for a compilation unit we should fold all of its functions and data
// declarations
std::any EtaConstantFolder::visit(ir::CompilationUnit &node) {
  return ir::CompilationUnit(
      node.name, visit_list<ir::DataDeclaration>(*this, node.dataDeclarations),
      visit_list<ir::FunctionDeclaration>(*this, node.functionDeclarations));
}

// an integer literal is already a constant
std::any EtaConstantFolder::visit(ir::IntegerLiteral &node) {
  return static_cast<std::shared_ptr<ir::Expression>>(
      std::make_shared<ir::IntegerLiteral>(node));
}

// a temporary can't be folded
std::any EtaConstantFolder::visit(ir::Temporary &node) {
  return static_cast<std::shared_ptr<ir::Expression>>(
      std::make_shared<ir::Temporary>(node));
}

int64_t hmul(int64_t a, int64_t b) {
  // Cast to 128-bit, multiply, shift the lower 64 bits away, and cast back
  __int128_t product = static_cast<__int128_t>(a) * b;
  return static_cast<int64_t>(product >> 64);
}

// the default action for the constant folder on binary expressions
inline std::shared_ptr<ir::Expression> default_action(
    const std::shared_ptr<ir::Expression> &left_lowered,
    const std::shared_ptr<ir::Expression> &right_lowered,
    ir::BinaryOperator op) {
  return static_cast<std::shared_ptr<ir::Expression>>(
      std::make_shared<ir::BinaryExpression>(left_lowered, right_lowered, op));
}

std::any EtaConstantFolder::visit(ir::BinaryExpression &node) {
  // fold the subexpressions
  auto left_lowered =
      std::any_cast<std::shared_ptr<ir::Expression>>(node.left->accept(*this));
  auto right_lowered =
      std::any_cast<std::shared_ptr<ir::Expression>>(node.right->accept(*this));
  // check if the lowered subexpressions are constant
  auto is_constant_left =
      std::dynamic_pointer_cast<ir::IntegerLiteral>(left_lowered);
  auto is_constant_right =
      std::dynamic_pointer_cast<ir::IntegerLiteral>(right_lowered);
  // check if both expressions are folded to constant
  if (is_constant_left && is_constant_right) {
    // extract the constants and compute the result
    auto left_val = is_constant_left->value;
    auto right_val = is_constant_right->value;
    int64_t retVal;
    switch (node.op) {
      case ir::BinaryOperator::Add:
        retVal = left_val + right_val;
        break;
      case ir::BinaryOperator::Sub:
        retVal = left_val - right_val;
        break;
      case ir::BinaryOperator::Mul:
        retVal = left_val * right_val;
        break;
      case ir::BinaryOperator::Hmul:
        retVal = hmul(left_val, right_val);
        break;
      case ir::BinaryOperator::Div:
        // if a division by zero is encountered, don't lower
        if (right_val == 0) {
          return default_action(left_lowered, right_lowered, node.op);
        }
        retVal = left_val / right_val;
        break;
      case ir::BinaryOperator::Mod:
        // if a division by zero is encountered, don't lower
        if (right_val == 0) {
          return default_action(left_lowered, right_lowered, node.op);
        }
        retVal = left_val % right_val;
        break;
      case ir::BinaryOperator::And:
        retVal = left_val == 1 && right_val == 1 ? 1 : 0;
        break;
      case ir::BinaryOperator::Or:
        retVal = left_val == 1 || right_val == 1 ? 1 : 0;
        break;
      case ir::BinaryOperator::Xor:
        retVal = left_val != right_val ? 1 : 0;
        break;
      case ir::BinaryOperator::Lshift:
        retVal = static_cast<uint64_t>(left_val)
                 << static_cast<uint64_t>(right_val);
        break;
      case ir::BinaryOperator::Rshift:
        retVal =
            static_cast<uint64_t>(left_val) >> static_cast<uint64_t>(right_val);
        break;
      case ir::BinaryOperator::Arshift:
        retVal = left_val >> right_val;
        break;
      case ir::BinaryOperator::Eq:
        retVal = left_val == right_val;
        break;
      case ir::BinaryOperator::Neq:
        retVal = left_val != right_val;
        break;
      case ir::BinaryOperator::Lt:
        retVal = left_val < right_val;
        break;
      case ir::BinaryOperator::Ult:
        retVal =
            static_cast<uint64_t>(left_val) < static_cast<uint64_t>(right_val);
        break;
      case ir::BinaryOperator::Gt:
        retVal = left_val > right_val;
        break;
      case ir::BinaryOperator::Leq:
        retVal = left_val <= right_val;
        break;
      case ir::BinaryOperator::Geq:
        retVal = left_val >= right_val;
        break;
    }
    return static_cast<std::shared_ptr<ir::Expression>>(
        std::make_shared<ir::IntegerLiteral>(retVal));
  } else {
    return default_action(left_lowered, right_lowered, node.op);
  }
}

std::any EtaConstantFolder::visit(ir::MemoryAccess &node) {
  // constant-fold the inner expression
  return static_cast<std::shared_ptr<ir::Expression>>(
      std::make_shared<ir::MemoryAccess>(
          std::any_cast<std::shared_ptr<ir::Expression>>(
              node.expr->accept(*this))));
}

std::any EtaConstantFolder::visit(ir::FunctionCall &node) {
  // constant-fold each of the function arguments
  return static_cast<std::shared_ptr<ir::Expression>>(
      std::make_shared<ir::FunctionCall>(
          node.function,
          visit_shared_list<ir::Expression>(*this, node.arguments)));
}

std::any EtaConstantFolder::visit(ir::Address &node) {
  // taking an address can't be folded
  return static_cast<std::shared_ptr<ir::Expression>>(
      std::make_shared<ir::Address>(node));
}

std::any EtaConstantFolder::visit(ir::ExpressionSequence &node) {
  return static_cast<std::shared_ptr<ir::Expression>>(
      std::make_shared<ir::ExpressionSequence>(
          std::any_cast<std::shared_ptr<ir::Statement>>(
              node.statement->accept(*this)),
          std::any_cast<std::shared_ptr<ir::Expression>>(
              node.expression->accept(*this))));
}

std::any EtaConstantFolder::visit(ir::Assignment &node) {
  if (auto *temp_ptr = std::get_if<ir::Temporary>(&node.dest)) {
    return static_cast<std::shared_ptr<ir::Statement>>(
        std::make_shared<ir::Assignment>(
            *temp_ptr, std::any_cast<std::shared_ptr<ir::Expression>>(
                           node.expression->accept(*this))));
  } else if (auto *mem_access_ptr = std::get_if<ir::MemoryAccess>(&node.dest)) {
    return static_cast<std::shared_ptr<ir::Statement>>(
        std::make_shared<ir::Assignment>(
            ir::MemoryAccess(std::any_cast<std::shared_ptr<ir::Expression>>(
                mem_access_ptr->expr->accept(*this))),
            std::any_cast<std::shared_ptr<ir::Expression>>(
                node.expression->accept(*this))));
  } else {
    throw std::runtime_error(
        "EtaConstantFolder::visit(ir::Assignment &node): impossible branch "
        "encountered");
  }
}

std::any EtaConstantFolder::visit(ir::StatementSequence &node) {
  // fold each of the statements
  return static_cast<std::shared_ptr<ir::Statement>>(
      std::make_shared<ir::StatementSequence>(
          visit_shared_list<ir::Statement>(*this, node.statements)));
}

std::any EtaConstantFolder::visit(ir::Jump &node) {
  return static_cast<std::shared_ptr<ir::Statement>>(
      std::make_shared<ir::Jump>(node));
}

std::any EtaConstantFolder::visit(ir::Label &node) {
  return static_cast<std::shared_ptr<ir::Statement>>(
      std::make_shared<ir::Label>(node));
}

std::any EtaConstantFolder::visit(ir::ConditionalJump &node) {
  // constant-fold the condition first
  auto folded_condition = std::any_cast<std::shared_ptr<ir::Expression>>(
      node.condition->accept(*this));
  auto is_condition_constant =
      std::dynamic_pointer_cast<ir::IntegerLiteral>(folded_condition);
  if (is_condition_constant) {
    // if the condition is constant, then simply the conditional jump to an
    // unconditional ne
    auto condition_value = is_condition_constant->value;
    return static_cast<std::shared_ptr<ir::Statement>>(
        std::make_shared<ir::Jump>(std::make_shared<ir::Address>(
            condition_value != 0 ? node.ifLabelName : node.elseLabelName)));
  } else {
    return static_cast<std::shared_ptr<ir::Statement>>(
        std::make_shared<ir::ConditionalJump>(
            folded_condition, node.ifLabelName, node.elseLabelName));
  }
}

std::any EtaConstantFolder::visit(ir::CallStatement &node) {
  // fold the arguments
  return static_cast<std::shared_ptr<ir::Statement>>(
      std::make_shared<ir::CallStatement>(
          node.function,
          visit_shared_list<ir::Expression>(*this, node.arguments),
          node.numReturns));
}

std::any EtaConstantFolder::visit(ir::ReturnStatement &node) {
  return static_cast<std::shared_ptr<ir::Statement>>(
      std::make_shared<ir::ReturnStatement>(
          visit_shared_list<ir::Expression>(*this, node.results)));
}

std::any EtaConstantFolder::visit(ir::Expression &node) {
  return node.accept(*this);
}
std::any EtaConstantFolder::visit(ir::IRNode &node) {
  return node.accept(*this);
}
std::any EtaConstantFolder::visit(ir::Statement &node) {
  return node.accept(*this);
}
}  // namespace ssy38::my_compiler::visitor