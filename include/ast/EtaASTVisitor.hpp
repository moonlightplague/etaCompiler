#pragma once

#include <any>
#include <stdexcept>

// forward declarations
namespace ssy38::my_compiler::ast {
struct ASTNode;

// Expressions
struct Expression;
struct UnaryExpression;
struct BinaryExpression;
struct ArrayInitializerExpression;
struct FunctionApplicationExpression;
struct LengthExpression;
struct IdentifierExpression;
struct BoolLiteralExpression;
struct IntLiteralExpression;
struct StringLiteralExpression;
struct CharLiteralExpression;
struct NullLiteralExpression;
struct FieldAccessExpression;

// Type nodes
struct EtaTypeNode;
struct BaseTypeNode;
struct ArrayTypeNode;
struct RecordTypeNode;

// Statements
struct Statement;
struct ReturnStatement;
struct IfStatement;
struct WhileStatement;
struct BlockStatement;
struct ProcedureCall;
struct DeclarationStatement;
struct AssignmentStatement;
struct BreakStatement;

// Declarations
struct Definition;
struct GlobalVariableDeclaration;
struct MethodInterface;
struct Method;
struct RecordDefinition;
struct Use;

// Lvalue nodes
struct LvalueNode;
struct DeclarationLvalueNode;
struct WildcardLvalueNode;
struct LvalueIdentifierNode;
struct LvalueArrayNode;
struct LvalueRecordNode;

// Top-level
struct Program;
struct Interface;
}  // namespace ssy38::my_compiler::ast

namespace ssy38::my_compiler::visitor {
/**
 * An annotate visitor abstract class for the Eta AST.
 * An annotate visitor annotates the Eta AST and doesn't return anything.
 */
class EtaASTAnnotateVisitor {
 public:
  virtual ~EtaASTAnnotateVisitor() = default;

  virtual void visit(ast::ASTNode& ast) = 0;

  // Expressions
  virtual void visit(ast::Expression& expr) = 0;
  virtual void visit(ast::UnaryExpression& expr) = 0;
  virtual void visit(ast::BinaryExpression& expr) = 0;
  virtual void visit(ast::ArrayInitializerExpression& expr) = 0;
  virtual void visit(ast::FunctionApplicationExpression& expr) = 0;
  virtual void visit(ast::LengthExpression& expr) = 0;
  virtual void visit(ast::IdentifierExpression& expr) = 0;
  virtual void visit(ast::BoolLiteralExpression& expr) = 0;
  virtual void visit(ast::IntLiteralExpression& expr) = 0;
  virtual void visit(ast::StringLiteralExpression& expr) = 0;
  virtual void visit(ast::CharLiteralExpression& expr) = 0;
  virtual void visit(ast::NullLiteralExpression& expr) = 0;
  virtual void visit(ast::FieldAccessExpression& expr) = 0;

  // Type nodes
  virtual void visit(ast::EtaTypeNode& type_node) = 0;
  virtual void visit(ast::BaseTypeNode& type_node) = 0;
  virtual void visit(ast::ArrayTypeNode& type_node) = 0;
  virtual void visit(ast::RecordTypeNode& type_node) = 0;

  // Statements
  virtual void visit(ast::Statement& stmt) = 0;
  virtual void visit(ast::ReturnStatement& stmt) = 0;
  virtual void visit(ast::IfStatement& stmt) = 0;
  virtual void visit(ast::WhileStatement& stmt) = 0;
  virtual void visit(ast::BlockStatement& stmt) = 0;
  virtual void visit(ast::ProcedureCall& stmt) = 0;
  virtual void visit(ast::DeclarationStatement& stmt) = 0;
  virtual void visit(ast::AssignmentStatement& stmt) = 0;
  virtual void visit(ast::BreakStatement& stmt) = 0;

  // Declarations / Definitions
  virtual void visit(ast::Definition& def) = 0;
  virtual void visit(ast::Use& use) = 0;
  virtual void visit(ast::GlobalVariableDeclaration& decl) = 0;
  virtual void visit(ast::MethodInterface& method_interface) = 0;
  virtual void visit(ast::Method& method) = 0;
  virtual void visit(ast::RecordDefinition& node) = 0;

  // Lvalue nodes
  virtual void visit(ast::LvalueNode& lvalue_node) = 0;
  virtual void visit(ast::DeclarationLvalueNode& lvalue_node) = 0;
  virtual void visit(ast::WildcardLvalueNode& lvalue_node) = 0;
  virtual void visit(ast::LvalueIdentifierNode& lvalue_node) = 0;
  virtual void visit(ast::LvalueArrayNode& lvalue_node) = 0;
  virtual void visit(ast::LvalueRecordNode& lvalue_node) = 0;

  // Top-level
  virtual void visit(ast::Program& program) = 0;
  virtual void visit(ast::Interface& interface) = 0;
};

/**
 * Default base annotate visitor implementation.
 */
class EtaASTBaseAnnotateVisitor : public EtaASTAnnotateVisitor {
  void visit(ast::ASTNode& ast) override {}

  // Expressions
  void visit(ast::Expression& expr) override {}
  void visit(ast::UnaryExpression& expr) override {}
  void visit(ast::BinaryExpression& expr) override {}
  void visit(ast::ArrayInitializerExpression& expr) override {}
  void visit(ast::FunctionApplicationExpression& expr) override {}
  void visit(ast::LengthExpression& expr) override {}
  void visit(ast::IdentifierExpression& expr) override {}
  void visit(ast::BoolLiteralExpression& expr) override {}
  void visit(ast::IntLiteralExpression& expr) override {}
  void visit(ast::StringLiteralExpression& expr) override {}
  void visit(ast::CharLiteralExpression& expr) override {}
  void visit(ast::NullLiteralExpression& expr) override {}
  void visit(ast::FieldAccessExpression& expr) override {}

  // Type nodes
  void visit(ast::EtaTypeNode& type_node) override {}
  void visit(ast::BaseTypeNode& type_node) override {}
  void visit(ast::ArrayTypeNode& type_node) override {}
  void visit(ast::RecordTypeNode& type_node) override {}

  // Statements
  void visit(ast::Statement& stmt) override {}
  void visit(ast::ReturnStatement& stmt) override {}
  void visit(ast::IfStatement& stmt) override {}
  void visit(ast::WhileStatement& stmt) override {}
  void visit(ast::BlockStatement& stmt) override {}
  void visit(ast::ProcedureCall& stmt) override {}
  void visit(ast::DeclarationStatement& stmt) override {}
  void visit(ast::AssignmentStatement& stmt) override {}
  void visit(ast::BreakStatement& stmt) override {}

  // Declarations / Definitions
  void visit(ast::Definition& def) override {}
  void visit(ast::Use& use) override {}
  void visit(ast::GlobalVariableDeclaration& decl) override {}
  void visit(ast::MethodInterface& method_interface) override {}
  void visit(ast::Method& method) override {}
  void visit(ast::RecordDefinition& node) override {}

  // Lvalue nodes
  void visit(ast::LvalueNode& lvalue_node) override {}
  void visit(ast::DeclarationLvalueNode& lvalue_node) override {}
  void visit(ast::WildcardLvalueNode& lvalue_node) override {}
  void visit(ast::LvalueIdentifierNode& lvalue_node) override {}
  void visit(ast::LvalueArrayNode& lvalue_node) override {}
  void visit(ast::LvalueRecordNode& lvalue_node) override {}

  // Top-level
  void visit(ast::Program& program) override {}
  void visit(ast::Interface& interface) override {}
};

/**
 * A build visitor abstract class for the Eta AST.
 * A build visitor visits the Eta AST and builds/returns a new object.
 */
class EtaASTBuildVisitor {
 public:
  virtual ~EtaASTBuildVisitor() = default;

  virtual std::any visit(const ast::ASTNode& ast) const = 0;

  // Expressions
  virtual std::any visit(const ast::Expression& expr) const = 0;
  virtual std::any visit(const ast::UnaryExpression& expr) const = 0;
  virtual std::any visit(const ast::BinaryExpression& expr) const = 0;
  virtual std::any visit(const ast::ArrayInitializerExpression& expr) const = 0;
  virtual std::any visit(const ast::FunctionApplicationExpression& expr) const = 0;
  virtual std::any visit(const ast::LengthExpression& expr) const = 0;
  virtual std::any visit(const ast::IdentifierExpression& expr) const = 0;
  virtual std::any visit(const ast::BoolLiteralExpression& expr) const = 0;
  virtual std::any visit(const ast::IntLiteralExpression& expr) const = 0;
  virtual std::any visit(const ast::StringLiteralExpression& expr) const = 0;
  virtual std::any visit(const ast::CharLiteralExpression& expr) const = 0;
  virtual std::any visit(const ast::NullLiteralExpression& expr) const = 0;
  virtual std::any visit(const ast::FieldAccessExpression& expr) const = 0;

  // Type nodes
  virtual std::any visit(const ast::EtaTypeNode& type_node) const = 0;
  virtual std::any visit(const ast::BaseTypeNode& type_node) const = 0;
  virtual std::any visit(const ast::ArrayTypeNode& type_node) const = 0;
  virtual std::any visit(const ast::RecordTypeNode& type_node) const = 0;

  // Statements
  virtual std::any visit(const ast::Statement& stmt) const = 0;
  virtual std::any visit(const ast::ReturnStatement& stmt) const = 0;
  virtual std::any visit(const ast::IfStatement& stmt) const = 0;
  virtual std::any visit(const ast::WhileStatement& stmt) const = 0;
  virtual std::any visit(const ast::BlockStatement& stmt) const = 0;
  virtual std::any visit(const ast::ProcedureCall& stmt) const = 0;
  virtual std::any visit(const ast::DeclarationStatement& stmt) const = 0;
  virtual std::any visit(const ast::AssignmentStatement& stmt) const = 0;
  virtual std::any visit(const ast::BreakStatement& stmt) const = 0;

  // Declarations / Definitions
  virtual std::any visit(const ast::Definition& def) const = 0;
  virtual std::any visit(const ast::Use& use) const = 0;
  virtual std::any visit(const ast::GlobalVariableDeclaration& decl) const = 0;
  virtual std::any visit(const ast::MethodInterface& method_interface) const = 0;
  virtual std::any visit(const ast::Method& method) const = 0;
  virtual std::any visit(const ast::RecordDefinition& node) const = 0;

  // Lvalue nodes
  virtual std::any visit(const ast::LvalueNode& lvalue_node) const = 0;
  virtual std::any visit(const ast::DeclarationLvalueNode& lvalue_node) const = 0;
  virtual std::any visit(const ast::WildcardLvalueNode& lvalue_node) const = 0;
  virtual std::any visit(const ast::LvalueIdentifierNode& lvalue_node) const = 0;
  virtual std::any visit(const ast::LvalueArrayNode& lvalue_node) const = 0;
  virtual std::any visit(const ast::LvalueRecordNode& lvalue_node) const = 0;

  // Top-level
  virtual std::any visit(const ast::Program& program) const = 0;
  virtual std::any visit(const ast::Interface& interface) const = 0;
};

/**
 * Default base build visitor implementation.
 */
class EtaASTBaseBuildVisitor : public EtaASTBuildVisitor {
 public:
  std::any visit(const ast::ASTNode& ast) const override {
    throw std::runtime_error("not implemented");
  }

  // Expressions
  std::any visit(const ast::Expression& expr) const override {
    throw std::runtime_error("not implemented");
  }
  std::any visit(const ast::UnaryExpression& expr) const override {
    throw std::runtime_error("not implemented");
  }
  std::any visit(const ast::BinaryExpression& expr) const override {
    throw std::runtime_error("not implemented");
  }
  std::any visit(const ast::ArrayInitializerExpression& expr) const override {
    throw std::runtime_error("not implemented");
  }
  std::any visit(const ast::FunctionApplicationExpression& expr) const override {
    throw std::runtime_error("not implemented");
  }
  std::any visit(const ast::LengthExpression& expr) const override {
    throw std::runtime_error("not implemented");
  }
  std::any visit(const ast::IdentifierExpression& expr) const override {
    throw std::runtime_error("not implemented");
  }
  std::any visit(const ast::BoolLiteralExpression& expr) const override {
    throw std::runtime_error("not implemented");
  }
  std::any visit(const ast::IntLiteralExpression& expr) const override {
    throw std::runtime_error("not implemented");
  }
  std::any visit(const ast::StringLiteralExpression& expr) const override {
    throw std::runtime_error("not implemented");
  }
  std::any visit(const ast::CharLiteralExpression& expr) const override {
    throw std::runtime_error("not implemented");
  }
  std::any visit(const ast::NullLiteralExpression& expr) const override {
    throw std::runtime_error("not implemented");
  }
  std::any visit(const ast::FieldAccessExpression& expr) const override {
    throw std::runtime_error("not implemented");
  }

  // Type nodes
  std::any visit(const ast::EtaTypeNode& type_node) const override {
    throw std::runtime_error("not implemented");
  }
  std::any visit(const ast::BaseTypeNode& type_node) const override {
    throw std::runtime_error("not implemented");
  }
  std::any visit(const ast::ArrayTypeNode& type_node) const override {
    throw std::runtime_error("not implemented");
  }
  std::any visit(const ast::RecordTypeNode& type_node) const override {
    throw std::runtime_error("not implemented");
  }

  // Statements
  std::any visit(const ast::Statement& stmt) const override {
    throw std::runtime_error("not implemented");
  }
  std::any visit(const ast::ReturnStatement& stmt) const override {
    throw std::runtime_error("not implemented");
  }
  std::any visit(const ast::IfStatement& stmt) const override {
    throw std::runtime_error("not implemented");
  }
  std::any visit(const ast::WhileStatement& stmt) const override {
    throw std::runtime_error("not implemented");
  }
  std::any visit(const ast::BlockStatement& stmt) const override {
    throw std::runtime_error("not implemented");
  }
  std::any visit(const ast::ProcedureCall& stmt) const override {
    throw std::runtime_error("not implemented");
  }
  std::any visit(const ast::DeclarationStatement& stmt) const override {
    throw std::runtime_error("not implemented");
  }
  std::any visit(const ast::AssignmentStatement& stmt) const override {
    throw std::runtime_error("not implemented");
  }
  std::any visit(const ast::BreakStatement& stmt) const override {
    throw std::runtime_error("not implemented");
  }

  // Declarations / Definitions
  std::any visit(const ast::Definition& def) const override {
    throw std::runtime_error("not implemented");
  }
  std::any visit(const ast::Use& use) const override {
    throw std::runtime_error("not implemented");
  }
  std::any visit(const ast::GlobalVariableDeclaration& decl) const override {
    throw std::runtime_error("not implemented");
  }
  std::any visit(const ast::MethodInterface& method_interface) const override {
    throw std::runtime_error("not implemented");
  }
  std::any visit(const ast::Method& method) const override {
    throw std::runtime_error("not implemented");
  }
  std::any visit(const ast::RecordDefinition& node) const override {
    throw std::runtime_error("not implemented");
  }

  // Lvalue nodes
  std::any visit(const ast::LvalueNode& lvalue_node) const override {
    throw std::runtime_error("not implemented");
  }
  std::any visit(const ast::DeclarationLvalueNode& lvalue_node) const override {
    throw std::runtime_error("not implemented");
  }
  std::any visit(const ast::WildcardLvalueNode& lvalue_node) const override {
    throw std::runtime_error("not implemented");
  }
  std::any visit(const ast::LvalueIdentifierNode& lvalue_node) const override {
    throw std::runtime_error("not implemented");
  }
  std::any visit(const ast::LvalueArrayNode& lvalue_node) const override {
    throw std::runtime_error("not implemented");
  }
  std::any visit(const ast::LvalueRecordNode& lvalue_node) const override {
    throw std::runtime_error("not implemented");
  }

  // Top-level
  std::any visit(const ast::Program& program) const override {
    throw std::runtime_error("not implemented");
  }
  std::any visit(const ast::Interface& interface) const override {
    throw std::runtime_error("not implemented");
  }
};
}  // namespace ssy38::my_compiler::visitor