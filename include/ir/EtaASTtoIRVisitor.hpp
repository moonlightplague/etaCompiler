#pragma once

#include "../ast/EtaASTVisitor.hpp"
#include "../ast/types.hpp"
#include "ir.hpp"

namespace ssy38::my_compiler::visitor {
/**
 * A visitor that translates the Eta AST into an MIR tree.
 */
class EtaASTtoIRVisitor : public EtaASTBaseBuildVisitor {
 public:
  std::any visit(const ast::ASTNode& ast) const override;

  // Expressions
  std::any visit(const ast::Expression& expr) const override;
  std::any visit(const ast::UnaryExpression& expr) const override;
  std::any visit(const ast::BinaryExpression& expr) const override;
  std::any visit(const ast::ArrayInitializerExpression& expr) const override;
  std::any visit(const ast::FunctionApplicationExpression& expr) const override;
  std::any visit(const ast::LengthExpression& expr) const override;
  std::any visit(const ast::IdentifierExpression& expr) const override;
  std::any visit(const ast::BoolLiteralExpression& expr) const override;
  std::any visit(const ast::IntLiteralExpression& expr) const override;
  std::any visit(const ast::StringLiteralExpression& expr) const override;
  std::any visit(const ast::CharLiteralExpression& expr) const override;
  std::any visit(const ast::NullLiteralExpression& expr) const override;
  std::any visit(const ast::FieldAccessExpression& expr) const override;

  // Type nodes
  std::any visit(const ast::EtaTypeNode& type_node) const override;
  std::any visit(const ast::BaseTypeNode& type_node) const override;
  std::any visit(const ast::ArrayTypeNode& type_node) const override;
  std::any visit(const ast::RecordTypeNode& type_node) const override;

  // Statements
  std::any visit(const ast::Statement& stmt) const override;
  std::any visit(const ast::ReturnStatement& stmt) const override;
  std::any visit(const ast::IfStatement& stmt) const override;
  std::any visit(const ast::WhileStatement& stmt) const override;
  std::any visit(const ast::BlockStatement& stmt) const override;
  std::any visit(const ast::ProcedureCall& stmt) const override;
  std::any visit(const ast::DeclarationStatement& stmt) const override;
  std::any visit(const ast::AssignmentStatement& stmt) const override;
  std::any visit(const ast::BreakStatement& stmt) const override;

  // Declarations / Definitions
  std::any visit(const ast::Definition& def) const override;
  std::any visit(const ast::Use& use) const override;
  std::any visit(const ast::GlobalVariableDeclaration& decl) const override;
  std::any visit(const ast::MethodInterface& method_interface) const override;
  std::any visit(const ast::Method& method) const override;

  // Lvalue nodes
  std::any visit(const ast::LvalueNode& lvalue_node) const override;
  std::any visit(const ast::DeclarationLvalueNode& lvalue_node) const override;
  std::any visit(const ast::WildcardLvalueNode& lvalue_node) const override;
  std::any visit(const ast::LvalueIdentifierNode& lvalue_node) const override;
  std::any visit(const ast::LvalueArrayNode& lvalue_node) const override;
  std::any visit(const ast::LvalueRecordNode& lvalue_node) const override;

  // Top-level
  std::any visit(const ast::Program& program) const override;

private:
    mutable std::optional<std::string> continuation = std::nullopt;
};
}  // namespace ssy38::my_compiler::visitor
