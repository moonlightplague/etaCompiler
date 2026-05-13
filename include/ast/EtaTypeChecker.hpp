#pragma once
#include <utility>
#include <map>
#include <set>

#include "ast.hpp"
#include "EtaASTVisitor.hpp"
#include "types.hpp"

namespace ssy38::my_compiler::types {

using InterfaceMap = std::map<std::string, ast::Program* const>;

class FirstPassTypeChecker : public visitor::EtaASTBaseAnnotateVisitor {
public:
    explicit FirstPassTypeChecker(InterfaceMap interfaces): interfaces(std::move(interfaces)) {}
    // Declarations
    void visit(ast::Definition& defn) override;
    void visit(ast::GlobalVariableDeclaration& decl) override;
    void visit(ast::MethodInterface& method_interface) override;
    void visit(ast::Method& method) override;
    void visit(ast::RecordDefinition& definition) override;
    void visit(ast::Use& use) override;

    // Top-level
    void visit(ast::Program& program) override;
private:
    InterfaceMap interfaces;
};

class SecondPassTypeChecker : public visitor::EtaASTBaseAnnotateVisitor {
public:
    explicit SecondPassTypeChecker(InterfaceMap interfaces, bool isProgram = false): isProgram(isProgram), interfaces(std::move(interfaces)) {}
    // Declarations
    void visit(ast::Definition& defn) override;
    void visit(ast::GlobalVariableDeclaration& decl) override;
    void visit(ast::MethodInterface& method_interface) override;
    void visit(ast::Method& method) override;
    void visit(ast::RecordDefinition& definition) override;
    void visit(ast::Use& use) override;

    // Top-level
    void visit(ast::Program& program) override;
private:
    bool isProgram;
    InterfaceMap interfaces;
};

/**
 * A visitor that type checks an Eta AST.
 */
class ThirdPassTypeChecker : public visitor::EtaASTBaseAnnotateVisitor {
 public:
  /**
   * The enum that defines which pass the type checker is currently in.
   */

  GammaType output;
  explicit ThirdPassTypeChecker(GammaType input) : input(std::move(input)) {}

  // Expressions
  void visit(ast::Expression& expr) override;
  void visit(ast::UnaryExpression& expr) override;
  void visit(ast::BinaryExpression& expr) override;
  void visit(ast::ArrayInitializerExpression& expr) override;
  void visit(ast::FunctionApplicationExpression& expr) override;
  void visit(ast::LengthExpression& expr) override;
  void visit(ast::IdentifierExpression& expr) override;
  void visit(ast::BoolLiteralExpression& expr) override;
  void visit(ast::IntLiteralExpression& expr) override;
  void visit(ast::StringLiteralExpression& expr) override;
  void visit(ast::CharLiteralExpression& expr) override;
  void visit(ast::NullLiteralExpression& expr) override;
  void visit(ast::FieldAccessExpression& expr) override;

  // Statements
  void visit(ast::Statement& stmt) override;
  void visit(ast::ReturnStatement& stmt) override;
  void visit(ast::IfStatement& stmt) override;
  void visit(ast::WhileStatement& stmt) override;
  void visit(ast::BlockStatement& stmt) override;
  void visit(ast::ProcedureCall& stmt) override;
  void visit(ast::DeclarationStatement& stmt) override;
  void visit(ast::AssignmentStatement& stmt) override;
  void visit(ast::BreakStatement& stmt) override;

  // Declarations
  void visit(ast::Definition& defn) override;
  void visit(ast::Use& use) override;
  void visit(ast::GlobalVariableDeclaration& decl) override;
  void visit(ast::MethodInterface& method_interface) override;
  void visit(ast::Method& method) override;
  void visit(ast::RecordDefinition& definition) override;

  // Top-level
  void visit(ast::Program& program) override;

private:
  GammaType input;
};

/**
 * A helper visitor that annotates each EtaTypeNode with a OrdinaryType to be
 * used in type checking.
 */
class OrdinaryTypeAnnotatorVisitor : public visitor::EtaASTBaseAnnotateVisitor {
 public:

  OrdinaryTypeAnnotatorVisitor(const GammaType &input) : input(input) {}
  void visit(ast::EtaTypeNode& node) override;
  void visit(ast::BaseTypeNode& node) override;
  void visit(ast::ArrayTypeNode& node) override;
  void visit(ast::RecordTypeNode& node) override;

 private:
    GammaType input;
};

/**
 * A helper visitor to verify that index expressions in array declarations are
 * well-typed.
 */
class ArrayTypeIndexInDeclarationVisitor : public visitor::EtaASTBaseAnnotateVisitor {
 public:
  ArrayTypeIndexInDeclarationVisitor(
      const ArrayTypeIndexInDeclarationVisitor&) = delete;
  ArrayTypeIndexInDeclarationVisitor& operator=(
      const ArrayTypeIndexInDeclarationVisitor&) = delete;
  static ArrayTypeIndexInDeclarationVisitor& getInstance() {
    static ArrayTypeIndexInDeclarationVisitor instance;
    return instance;
  }

  void visit(ast::EtaTypeNode& node) override;
  void visit(ast::BaseTypeNode& node) override;
  void visit(ast::ArrayTypeNode& node) override;
  void visit(ast::RecordTypeNode& node) override;

 private:
  ArrayTypeIndexInDeclarationVisitor() {}
};

/**
 * A helper visitor that annotates each LvalueNode with an OrdinaryType such
 * that any rvalue assigned to it must have this type.
 */
class LValueTypeChecker : public visitor::EtaASTBaseAnnotateVisitor {
 public:
  LValueTypeChecker(const LValueTypeChecker&) = delete;
  LValueTypeChecker& operator=(const LValueTypeChecker&) = delete;
  static LValueTypeChecker& getInstance() {
    static LValueTypeChecker instance;
    return instance;
  }

  void visit(ast::LvalueNode& node) override;
  void visit(ast::LvalueIdentifierNode& node) override;
  void visit(ast::LvalueArrayNode& node) override;
  void visit(ast::WildcardLvalueNode& node) override;
  void visit(ast::DeclarationLvalueNode& node) override;
  void visit(ast::LvalueRecordNode& node) override;

 private:
  LValueTypeChecker() {}
};
}  // namespace ssy38::my_compiler::types