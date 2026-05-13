#include "EtaTrivialRegisterAllocationVisitor.hpp"

#include <algorithm>

#include "asm.hpp"
#include "utils.hpp"

namespace ssy38::my_compiler::visitor {

/**
 * Allocates register for the i'th operand in an abstract assembly instruction.
 *
 * @param i - the index of the operand in an abstract assembly instruction.
 * @return the register for the ith operand in an abstract assembly instruction.
 */
inline codegen::Register alloc_reg(int i) {
  if (i == 1) {
    return codegen::Register::R11;
  } else if (i == 2) {
    return codegen::Register::R10;
  } else if (i == 3) {
    return codegen::Register::R9;
  } else {
    throw std::runtime_error("Each operation should only have <= 3 registers");
  }
}

std::any EtaTrivialRegisterAllocationVisitor::visit(
    codegen::CommandSequence& node) {
  std::vector<std::shared_ptr<codegen::Command>> results;
  std::vector<std::size_t>
      frame_allocation_index_in_sequence;  // track the enter operations in
                                           // results vector
  for (const auto& command : node.commands) {
    if (auto p = std::dynamic_pointer_cast<codegen::Operation>(command)) {
      if (p->opcode == codegen::OpCode::ENTER) {
        frame_allocation_index_in_sequence.push_back(results.size());
      }
    }
    auto sequence =
        std::any_cast<codegen::CommandSequence>(command->accept(*this));
    results.insert(results.end(), sequence.commands.begin(),
                   sequence.commands.end());
  }
  if (frame_allocation_index_in_sequence.size() !=
      FUNCTION_VARIABLE_COUNT.size()) {
    throw std::runtime_error(
        "Inconformity between number of enter operations and number of "
        "functions");
  }
  int count = 0;
  for (const auto& index : frame_allocation_index_in_sequence) {
    if (auto p =
            std::dynamic_pointer_cast<codegen::Operation>(results[index])) {
      if (p->opcode == codegen::OpCode::ENTER) {
        p->operand1 = std::make_shared<codegen::ConstantOperand>(
            FUNCTION_VARIABLE_COUNT[count] % 2 == 0
                ? FUNCTION_VARIABLE_COUNT[count] * 8
                : FUNCTION_VARIABLE_COUNT[count] * 8 + 8);
        count++;
      } else
        throw std::runtime_error("enter operation index error");
    } else
      throw std::runtime_error("enter operation index error");
  }
  return codegen::CommandSequence(results);
}

std::any EtaTrivialRegisterAllocationVisitor::visit(
    codegen::ConstantOperand& node) {
  return node;
}
std::any EtaTrivialRegisterAllocationVisitor::visit(
    codegen::RegisterOperand& node) {
  return node;
}

std::any EtaTrivialRegisterAllocationVisitor::visit(
    codegen::TemporaryOperand& node) {
  return codegen::RegisterOperand(
      alloc_reg(REGISTER_TEMPORARY_INDICES[node.name]));
}

std::any EtaTrivialRegisterAllocationVisitor::visit(
    codegen::MemoryOperand& node) {
  std::optional<std::variant<codegen::ConstantOperand, codegen::RegisterOperand,
                             codegen::TemporaryOperand, codegen::LabelOperand>>
      translatedBase;
  if (node.base.has_value()) {
    translatedBase = std::visit(
        utils::overloaded{
            [](codegen::ConstantOperand& operand) {
              return std::variant<
                  codegen::ConstantOperand, codegen::RegisterOperand,
                  codegen::TemporaryOperand, codegen::LabelOperand>(operand);
            },
            [](codegen::RegisterOperand& operand) {
              return std::variant<
                  codegen::ConstantOperand, codegen::RegisterOperand,
                  codegen::TemporaryOperand, codegen::LabelOperand>(operand);
            },
            [this](codegen::TemporaryOperand& operand) {
              return std::variant<
                  codegen::ConstantOperand, codegen::RegisterOperand,
                  codegen::TemporaryOperand, codegen::LabelOperand>(
                  std::any_cast<codegen::RegisterOperand>(
                      operand.accept(*this)));
            },
            [](codegen::LabelOperand& operand) {
              return std::variant<
                  codegen::ConstantOperand, codegen::RegisterOperand,
                  codegen::TemporaryOperand, codegen::LabelOperand>(operand);
            }},
        node.base.value());
  } else {
    translatedBase = std::nullopt;
  }

  std::optional<
      std::variant<codegen::RegisterOperand, codegen::TemporaryOperand>>
      translatedIdx;
  if (node.idx.has_value()) {
    translatedIdx = std::visit(
        utils::overloaded{[](codegen::RegisterOperand& operand) {
                            return std::variant<codegen::RegisterOperand,
                                                codegen::TemporaryOperand>(
                                operand);
                          },
                          [this](codegen::TemporaryOperand& operand) {
                            return std::variant<codegen::RegisterOperand,
                                                codegen::TemporaryOperand>(
                                std::any_cast<codegen::RegisterOperand>(
                                    operand.accept(*this)));
                          }},
        node.idx.value());
  } else {
    translatedIdx = std::nullopt;
  }

  return codegen::MemoryOperand(node.size, translatedBase, translatedIdx,
                                node.scale, node.constant);
}

std::any EtaTrivialRegisterAllocationVisitor::visit(
    codegen::LabelOperand& node) {
  return node;
}

std::any EtaTrivialRegisterAllocationVisitor::visit(codegen::Directive& node) {
  if (node.text.ends_with("@function")) {
    STACK_TEMPORARY_INDICES.clear();
  }
  return codegen::CommandSequence({std::make_shared<codegen::Directive>(node)});
}

std::any EtaTrivialRegisterAllocationVisitor::visit(
    codegen::LabelDeclaration& node) {
  if (node.name == "END_OF_FUNC") {
    // if the node is an END_OF_FUNC flag, set the FUNCTION_VARIABLE_COUNT for
    // this function and delete itself.
    FUNCTION_VARIABLE_COUNT.push_back(
        static_cast<int64_t>(STACK_TEMPORARY_INDICES.size()));
    return codegen::CommandSequence({});
  }
  return codegen::CommandSequence(
      {std::make_shared<codegen::LabelDeclaration>(node)});
}

std::any EtaTrivialRegisterAllocationVisitor::visit(codegen::Operation& node) {
  REGISTER_TEMPORARY_INDICES.clear();
  if (node.opcode == codegen::OpCode::ENTER) {
    // if the node is an enter operation, return itself.
    return codegen::CommandSequence(
        {std::make_shared<codegen::Operation>(node)});
  }
  // check each operand in the operation and see if it is a temporary, if so,
  // add it to the vector
  std::vector<codegen::TemporaryOperand> temporary_operands;
  auto add_temporary_if_needed =
      [&temporary_operands](const codegen::TemporaryOperand& temp) {
        const bool exists = std::ranges::any_of(
            temporary_operands, [&temp](const codegen::TemporaryOperand& item) {
              return item.name == temp.name;
            });
        if (!exists) {
          temporary_operands.push_back(temp);
        }
      };
  for (const auto& operand : {node.operand1, node.operand2}) {
    if (operand) {
      if (const auto temp_ptr =
              std::dynamic_pointer_cast<codegen::TemporaryOperand>(operand)) {
        add_temporary_if_needed(*temp_ptr);
      } else if (const auto mem_ptr =
                     std::dynamic_pointer_cast<codegen::MemoryOperand>(
                         operand)) {
        if (mem_ptr->base.has_value()) {
          if (const auto base_temp = std::get_if<codegen::TemporaryOperand>(
                  &mem_ptr->base.value())) {
            add_temporary_if_needed(*base_temp);
          }
        }
        if (mem_ptr->idx.has_value()) {
          if (const auto idx_temp = std::get_if<codegen::TemporaryOperand>(
                  &mem_ptr->idx.value())) {
            add_temporary_if_needed(*idx_temp);
          }
        }
      }
    }
  }
  if (temporary_operands.size() > 3) {
    throw std::runtime_error(
        "Each operation should only have <= 3 temporary operands");
  }

  std::vector<std::shared_ptr<codegen::Command>> results;

  // shuttle from stack to registers
  for (const auto& temp : temporary_operands) {
    if (!REGISTER_TEMPORARY_INDICES.contains(temp.name)) {
      REGISTER_TEMPORARY_INDICES[temp.name] =
          REGISTER_TEMPORARY_INDICES.size() + 1;
    }
    if (!STACK_TEMPORARY_INDICES.contains(temp.name)) {
      STACK_TEMPORARY_INDICES[temp.name] = STACK_TEMPORARY_INDICES.size() + 1;
    }
    const int64_t temp_i = STACK_TEMPORARY_INDICES[temp.name];
    const int64_t current_i = REGISTER_TEMPORARY_INDICES[temp.name];

    results.emplace_back(std::make_shared<codegen::Operation>(
        codegen::OpCode::MOV,
        std::make_shared<codegen::RegisterOperand>(alloc_reg(current_i)),
        std::make_shared<codegen::MemoryOperand>(
            codegen::Size::QWORD_PTR,
            codegen::RegisterOperand(codegen::Register::RBP), std::nullopt,
            std::nullopt, codegen::ConstantOperand(-8 * temp_i))));
  }

  // translate the operation (replace temporaries with registers)
  auto translate_operand =
      [this](const std::shared_ptr<codegen::Operand>& operand)
      -> std::shared_ptr<codegen::Operand> {
    if (!operand) {
      return nullptr;
    }
    if (const auto constant_ptr =
            std::dynamic_pointer_cast<codegen::ConstantOperand>(operand)) {
      return std::make_shared<codegen::ConstantOperand>(*constant_ptr);
    }
    if (const auto register_ptr =
            std::dynamic_pointer_cast<codegen::RegisterOperand>(operand)) {
      return std::make_shared<codegen::RegisterOperand>(*register_ptr);
    }
    if (const auto temporary_ptr =
            std::dynamic_pointer_cast<codegen::TemporaryOperand>(operand)) {
      return std::make_shared<codegen::RegisterOperand>(
          std::any_cast<codegen::RegisterOperand>(
              temporary_ptr->accept(*this)));
    }
    if (const auto memory_ptr =
            std::dynamic_pointer_cast<codegen::MemoryOperand>(operand)) {
      return std::make_shared<codegen::MemoryOperand>(
          std::any_cast<codegen::MemoryOperand>(memory_ptr->accept(*this)));
    }
    if (const auto label_ptr =
            std::dynamic_pointer_cast<codegen::LabelOperand>(operand)) {
      return std::make_shared<codegen::LabelOperand>(*label_ptr);
    }
    throw std::runtime_error(
        "Unsupported operand type during register allocation");
  };

  const auto translated_operand1 = translate_operand(node.operand1);
  const auto translated_operand2 = translate_operand(node.operand2);
  if (translated_operand2 && !translated_operand1) {
    throw std::runtime_error("Invalid operation with operand2 but no operand1");
  }

  if (translated_operand1 && translated_operand2) {
    results.emplace_back(std::make_shared<codegen::Operation>(
        node.opcode, translated_operand1, translated_operand2));
  } else if (translated_operand1) {
    results.emplace_back(
        std::make_shared<codegen::Operation>(node.opcode, translated_operand1));
  } else {
    results.emplace_back(std::make_shared<codegen::Operation>(node.opcode));
  }

  // shuttle from registers to stack
  for (const auto& temp : temporary_operands) {
    const int64_t temp_i = STACK_TEMPORARY_INDICES[temp.name];
    const int64_t current_i = REGISTER_TEMPORARY_INDICES[temp.name];

    results.emplace_back(std::make_shared<codegen::Operation>(
        codegen::OpCode::MOV,
        std::make_shared<codegen::MemoryOperand>(
            codegen::Size::QWORD_PTR,
            codegen::RegisterOperand(codegen::Register::RBP), std::nullopt,
            std::nullopt, codegen::ConstantOperand(-8 * temp_i)),
        std::make_shared<codegen::RegisterOperand>(alloc_reg(current_i))));
  }

  return codegen::CommandSequence(results);
}

}  // namespace ssy38::my_compiler::visitor
