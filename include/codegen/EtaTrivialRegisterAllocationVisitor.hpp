#pragma once
#include <string>

#include "EtaASMVisitor.hpp"
#include "immer/detail/hamts/champ.hpp"
#include "immer/map.hpp"

namespace ssy38::my_compiler::visitor {
/**
 * A visitor that trivially allocates registers of abstract assembly (shuttles
 * registers on and off the stack).
 */
class EtaTrivialRegisterAllocationVisitor : public EtaASMBaseVisitor {
public:
  std::unordered_map<std::string, int64_t> STACK_TEMPORARY_INDICES; // current function's temporary name to stack frame index
  std::unordered_map<std::string, int64_t> REGISTER_TEMPORARY_INDICES; // current function's temporary name to alloc_reg index
  std::vector<int64_t> FUNCTION_VARIABLE_COUNT;

  std::any visit(codegen::CommandSequence& node) override;
  std::any visit(codegen::ConstantOperand& node) override;
  std::any visit(codegen::RegisterOperand& node) override;
  std::any visit(codegen::TemporaryOperand& node) override;
  std::any visit(codegen::MemoryOperand& node) override;
  std::any visit(codegen::LabelOperand& node) override;
  std::any visit(codegen::Directive& node) override;
  std::any visit(codegen::LabelDeclaration& node) override;
  std::any visit(codegen::Operation& node) override;
};
}