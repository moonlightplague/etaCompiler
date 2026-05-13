#pragma once

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>
#include <map>

#include "EtaASTVisitor.hpp"
#include "types.hpp"
#include "utils.hpp"

namespace ssy38::my_compiler::ast {
  /**
   * An AST Node that can be pretty-printed in an S-expression format.
   */
  struct ASTNode {
    virtual ~ASTNode() = default;

    size_t line;
    size_t charPositionInLine;

    /**
     * Pretty-print an AST Node in an S-expression format.
     *
     * @param indent - The indentation (number of spaces) before each
     * S-expression.
     *
     * @return The pretty-printed string of the AST Node in S-expression format.
     */
    virtual std::string toString(int indent) = 0;

    /**
     * Accepts an annotate visitor and performs double dispatch.
     *
     * @param visitor - the visitor to accept
     */
    virtual void accept(visitor::EtaASTAnnotateVisitor &visitor) = 0;

    /**
     * Accepts a build visitor and performs double dispatch.
     *
     * @param visitor - the visitor to accept
     * @return the created value
     */
    virtual std::any accept(const visitor::EtaASTBuildVisitor &visitor) const = 0;
  };

  struct Expression : public ASTNode {
    types::ExpressionTypeJudgement judgement;

    /**
     * Represents a Unary Operand.
     */
    enum class UnaryOperand { Minus, Bang };

    /**
     * Represents a Binary Operand.
     */
    enum class BinaryOperand {
      Multiply,
      HighMultiply,
      Divide,
      Mod,
      Plus,
      Minus,
      LessThan,
      LessThanEqual,
      GreaterThan,
      GreaterThanEqual,
      Equal,
      NotEqual,
      And,
      Or,
      ArrayAccess
    };
  };

  /**
   * The string representation of the unary operand.
   *
   * @param op - the operand
   * @return The string representation
   */
  std::string UnaryOperand2String(Expression::UnaryOperand op);

  /**
   * The string representation of the binary operand.
   *
   * @param op - the operand
   * @return The string representation
   */
  std::string BinaryOperand2String(Expression::BinaryOperand op);

  struct UnaryExpression final : public Expression {
    std::unique_ptr<Expression> expr;
    UnaryOperand op;

    UnaryExpression(const UnaryOperand op, std::unique_ptr<Expression> expr,
                    const size_t line, const size_t charPositionInLine)
      : expr(std::move(expr)), op(op) {
      this->line = line;
      this->charPositionInLine = charPositionInLine;
    }

    std::string toString(int indent) override;

    void accept(visitor::EtaASTAnnotateVisitor &visitor) override {
      visitor.visit(*this);
    }

    std::any accept(const visitor::EtaASTBuildVisitor &visitor) const override {
      return visitor.visit(*this);
    }
  };

  struct BinaryExpression final : public Expression {
    std::unique_ptr<Expression> left, right;
    BinaryOperand op;

    BinaryExpression(const BinaryOperand op, std::unique_ptr<Expression> left,
                     std::unique_ptr<Expression> right, const size_t line,
                     const size_t charPositionInLine)
      : left(std::move(left)), right(std::move(right)), op(op) {
      this->line = line;
      this->charPositionInLine = charPositionInLine;
    }

    std::string toString(int indent) override;

    void accept(visitor::EtaASTAnnotateVisitor &visitor) override {
      visitor.visit(*this);
    }

    std::any accept(const visitor::EtaASTBuildVisitor &visitor) const override {
      return visitor.visit(*this);
    }
  };

  struct ArrayInitializerExpression final : public Expression {
    std::vector<std::unique_ptr<Expression> > exprs;

    explicit ArrayInitializerExpression(
      std::vector<std::unique_ptr<Expression> > exprs, const size_t line,
      const size_t charPositionInLine)
      : exprs(std::move(exprs)) {
      this->line = line;
      this->charPositionInLine = charPositionInLine;
    }

    std::string toString(int indent) override;

    void accept(visitor::EtaASTAnnotateVisitor &visitor) override {
      visitor.visit(*this);
    }

    std::any accept(const visitor::EtaASTBuildVisitor &visitor) const override {
      return visitor.visit(*this);
    }
  };

  struct FieldAccessExpression final : public Expression {
    std::unique_ptr<Expression> expr;
    std::string id;
    FieldAccessExpression(std::unique_ptr<Expression> expr,
                                  std::string id,
                                  const size_t line,
                                  const size_t charPositionInLine)
      : id(std::move(id)), expr(std::move(expr)) {
      this->line = line;
      this->charPositionInLine = charPositionInLine;
    }

    std::string toString(int indent) override;

    void accept(visitor::EtaASTAnnotateVisitor &visitor) override {
      visitor.visit(*this);
    }

    std::any accept(const visitor::EtaASTBuildVisitor &visitor) const override {
      return visitor.visit(*this);
    }
  };

  struct FunctionApplicationExpression final : public Expression {
    std::string id;
    std::vector<std::unique_ptr<Expression> > exprs;

    FunctionApplicationExpression(std::string id,
                                  std::vector<std::unique_ptr<Expression> > exprs,
                                  const size_t line,
                                  const size_t charPositionInLine)
      : id(std::move(id)), exprs(std::move(exprs)) {
      this->line = line;
      this->charPositionInLine = charPositionInLine;
    }

    std::string toString(int indent) override;

    void accept(visitor::EtaASTAnnotateVisitor &visitor) override {
      visitor.visit(*this);
    }

    std::any accept(const visitor::EtaASTBuildVisitor &visitor) const override {
      return visitor.visit(*this);
    }
  };

  struct LengthExpression final : public Expression {
    std::unique_ptr<Expression> expr;

    LengthExpression(std::unique_ptr<Expression> expr, const size_t line,
                     const size_t charPositionInLine)
      : expr(std::move(expr)) {
      this->line = line;
      this->charPositionInLine = charPositionInLine;
    }

    std::string toString(int indent) override;

    void accept(visitor::EtaASTAnnotateVisitor &visitor) override {
      visitor.visit(*this);
    }

    std::any accept(const visitor::EtaASTBuildVisitor &visitor) const override {
      return visitor.visit(*this);
    }
  };

  struct IdentifierExpression final : public Expression {
    std::string id;

    explicit IdentifierExpression(std::string id, const size_t line,
                                  const size_t charPositionInLine)
      : id(std::move(id)) {
      this->line = line;
      this->charPositionInLine = charPositionInLine;
    }

    std::string toString(int indent) override;

    void accept(visitor::EtaASTAnnotateVisitor &visitor) override {
      visitor.visit(*this);
    }

    std::any accept(const visitor::EtaASTBuildVisitor &visitor) const override {
      return visitor.visit(*this);
    }
  };

  struct BoolLiteralExpression final : public Expression {
    bool value;

    explicit BoolLiteralExpression(const bool value, const size_t line,
                                   const size_t charPositionInLine)
      : value(value) {
      this->line = line;
      this->charPositionInLine = charPositionInLine;
    }

    std::string toString(int indent) override;

    void accept(visitor::EtaASTAnnotateVisitor &visitor) override {
      visitor.visit(*this);
    }

    std::any accept(const visitor::EtaASTBuildVisitor &visitor) const override {
      return visitor.visit(*this);
    }
  };

  struct IntLiteralExpression final : public Expression {
    int64_t value;

    explicit IntLiteralExpression(const int64_t value, const size_t line,
                                  const size_t charPositionInLine)
      : value(value) {
      this->line = line;
      this->charPositionInLine = charPositionInLine;
    }

    std::string toString(int indent) override;

    void accept(visitor::EtaASTAnnotateVisitor &visitor) override {
      visitor.visit(*this);
    }

    std::any accept(const visitor::EtaASTBuildVisitor &visitor) const override {
      return visitor.visit(*this);
    }
  };

  struct NullLiteralExpression final : public Expression {
    explicit NullLiteralExpression(const size_t line, const size_t charPositionInLine) {
      this->line = line;
      this->charPositionInLine = charPositionInLine;
    }

    std::string toString(int indent) override;

    void accept(visitor::EtaASTAnnotateVisitor &visitor) override {
      visitor.visit(*this);
    }

    std::any accept(const visitor::EtaASTBuildVisitor &visitor) const override {
      return visitor.visit(*this);
    }
  };


  struct StringLiteralExpression final : public Expression {
    std::string value;
    std::vector<uint64_t> codepoints;

    explicit StringLiteralExpression(std::string value,
                                     std::vector<uint64_t> codepoints,
                                     const size_t line,
                                     const size_t charPositionInLine)
      : value(std::move(value)), codepoints(std::move(codepoints)) {
      this->line = line;
      this->charPositionInLine = charPositionInLine;
    }

    std::string toString(int indent) override;

    void accept(visitor::EtaASTAnnotateVisitor &visitor) override {
      visitor.visit(*this);
    }

    std::any accept(const visitor::EtaASTBuildVisitor &visitor) const override {
      return visitor.visit(*this);
    }
  };

  struct CharLiteralExpression final : public Expression {
    std::string value;
    uint64_t codepoint;

    explicit CharLiteralExpression(std::string value, uint64_t codepoint,
                                   const size_t line,
                                   const size_t charPositionInLine)
      : value(std::move(value)), codepoint(codepoint) {
      this->line = line;
      this->charPositionInLine = charPositionInLine;
    }

    std::string toString(int indent) override;

    void accept(visitor::EtaASTAnnotateVisitor &visitor) override {
      visitor.visit(*this);
    }

    std::any accept(const visitor::EtaASTBuildVisitor &visitor) const override {
      return visitor.visit(*this);
    }
  };

  struct EtaTypeNode : public ASTNode {
    std::shared_ptr<types::OrdinaryType>
    ordinaryType; // to obtain the ordinary type of EtaTypeNode (annotated by
    // OrdinaryTypeAnnotatorVisitor)
    types::GammaType
    input; // to type check expressions inside array declarations (annotated
    // by ArrayTypeIndexInDeclarationVisitor)

    std::vector<const Expression *>
    allocators; // to store the list of initialization expressions appearing in array types; e.g.
    // (int[1][2][]).allocators = {1, 2}
    /**
     * The base type enum class.
     */
    enum class BaseType { Integer, Boolean };

    void accept(visitor::EtaASTAnnotateVisitor &visitor) override {
      visitor.visit(*this);
    }

    std::any accept(const visitor::EtaASTBuildVisitor &visitor) const override {
      return visitor.visit(*this);
    }
  };

  struct BaseTypeNode final : public EtaTypeNode {
    BaseType type;

    explicit BaseTypeNode(const EtaTypeNode::BaseType type, const size_t line,
                          const size_t charPositionInLine)
      : type(type) {
      this->line = line;
      this->charPositionInLine = charPositionInLine;
      this->allocators = {};
    }

    std::string toString(int indent) override;

    void accept(visitor::EtaASTAnnotateVisitor &visitor) override {
      visitor.visit(*this);
    }

    std::any accept(const visitor::EtaASTBuildVisitor &visitor) const override {
      return visitor.visit(*this);
    }
  };

  struct ArrayTypeNode final : public EtaTypeNode {
    std::unique_ptr<Expression> dimension; // nullable
    std::unique_ptr<EtaTypeNode> subtype;

    explicit ArrayTypeNode(std::unique_ptr<EtaTypeNode> subtype,
                           std::unique_ptr<Expression> dimension,
                           const size_t line, const size_t charPositionInLine)
      : dimension(std::move(dimension)), subtype(std::move(subtype)) {
      this->line = line;
      this->charPositionInLine = charPositionInLine;
      if (this->subtype == nullptr) {
        this->allocators = {};
      } else {
        this->allocators = this->subtype->allocators;
      }
      if (this->dimension != nullptr) {
        this->allocators.insert(this->allocators.begin(), this->dimension.get());
      }
    }

    std::string toString(int indent) override;

    void accept(visitor::EtaASTAnnotateVisitor &visitor) override {
      visitor.visit(*this);
    }

    std::any accept(const visitor::EtaASTBuildVisitor &visitor) const override {
      return visitor.visit(*this);
    }
  };

  struct RecordTypeNode final : public EtaTypeNode {
    std::string name;
    RecordTypeNode(std::string name, const size_t line, const size_t charPositionInLine) : name(std::move(name)) {
      this->line = line ;
      this->charPositionInLine = charPositionInLine;
      this->allocators = {};
    }
    std::string toString(int indent) override;

    void accept(visitor::EtaASTAnnotateVisitor &visitor) override {
      visitor.visit(*this);
    }

    std::any accept(const visitor::EtaASTBuildVisitor &visitor) const override {
      return visitor.visit(*this);
    }
  };

  // --- Declarations and Statements ---

  struct Statement : public ASTNode {
    types::StatementTypeJudgement judgement;
  };

  struct ReturnStatement final : public Statement {
    std::vector<std::unique_ptr<Expression> > exprs;

    explicit ReturnStatement(std::vector<std::unique_ptr<Expression> > exprs,
                             const size_t line, const size_t charPositionInLine)
      : exprs(std::move(exprs)) {
      this->line = line;
      this->charPositionInLine = charPositionInLine;
    }

    std::string toString(int indent) override;

    void accept(visitor::EtaASTAnnotateVisitor &visitor) override {
      visitor.visit(*this);
    }

    std::any accept(const visitor::EtaASTBuildVisitor &visitor) const override {
      return visitor.visit(*this);
    }
  };

  struct IfStatement final : public Statement {
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> then_branch;
    std::unique_ptr<Statement> else_branch; // nullable

    IfStatement(std::unique_ptr<Expression> condition,
                std::unique_ptr<Statement> then_branch,
                std::unique_ptr<Statement> else_branch, const size_t line,
                const size_t charPositionInLine)
      : condition(std::move(condition)),
        then_branch(std::move(then_branch)),
        else_branch(std::move(else_branch)) {
      this->line = line;
      this->charPositionInLine = charPositionInLine;
    }

    std::string toString(int indent) override;

    void accept(visitor::EtaASTAnnotateVisitor &visitor) override {
      visitor.visit(*this);
    }

    std::any accept(const visitor::EtaASTBuildVisitor &visitor) const override {
      return visitor.visit(*this);
    }
  };

  struct WhileStatement final : public Statement {
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> body;

    WhileStatement(std::unique_ptr<Expression> condition,
                   std::unique_ptr<Statement> body, const size_t line,
                   const size_t charPositionInLine)
      : condition(std::move(condition)), body(std::move(body)) {
      this->line = line;
      this->charPositionInLine = charPositionInLine;
    }

    std::string toString(int indent) override;

    void accept(visitor::EtaASTAnnotateVisitor &visitor) override {
      visitor.visit(*this);
    }

    std::any accept(const visitor::EtaASTBuildVisitor &visitor) const override {
      return visitor.visit(*this);
    }
  };

  struct BlockStatement final : public Statement {
    std::vector<std::unique_ptr<Statement> > statements;

    explicit BlockStatement(std::vector<std::unique_ptr<Statement> > statements,
                            const size_t line, const size_t charPositionInLine)
      : statements(std::move(statements)) {
      this->line = line;
      this->charPositionInLine = charPositionInLine;
    }

    std::string toString(int indent) override;

    void accept(visitor::EtaASTAnnotateVisitor &visitor) override {
      visitor.visit(*this);
    }

    std::any accept(const visitor::EtaASTBuildVisitor &visitor) const override {
      return visitor.visit(*this);
    }
  };

  struct BreakStatement final : public Statement {
    std::vector<std::unique_ptr<Statement> > statements;

    explicit BreakStatement(
                            const size_t line, const size_t charPositionInLine)
       {
      this->line = line;
      this->charPositionInLine = charPositionInLine;
    }

    std::string toString(int indent) override;

    void accept(visitor::EtaASTAnnotateVisitor &visitor) override {
      visitor.visit(*this);
    }

    std::any accept(const visitor::EtaASTBuildVisitor &visitor) const override {
      return visitor.visit(*this);
    }
  };

  struct ProcedureCall final : public Statement {
    std::string id;
    std::vector<std::unique_ptr<Expression> > arguments;

    ProcedureCall(std::string id,
                  std::vector<std::unique_ptr<Expression> > arguments,
                  const size_t line, const size_t charPositionInLine)
      : id(std::move(id)), arguments(std::move(arguments)) {
      this->line = line;
      this->charPositionInLine = charPositionInLine;
    }

    std::string toString(int indent) override;

    void accept(visitor::EtaASTAnnotateVisitor &visitor) override {
      visitor.visit(*this);
    }

    std::any accept(const visitor::EtaASTBuildVisitor &visitor) const override {
      return visitor.visit(*this);
    }
  };

  using Declaration = std::pair<std::string, std::unique_ptr<ast::EtaTypeNode> >;

  struct DeclarationStatement final : public Statement {
    std::vector<Declaration> declarations;

    explicit DeclarationStatement(std::vector<Declaration> declarations,
                                  const size_t line,
                                  const size_t charPositionInLine)
      : declarations(std::move(declarations)) {
      this->line = line;
      this->charPositionInLine = charPositionInLine;
    }

    std::string toString(int indent) override;

    void accept(visitor::EtaASTAnnotateVisitor &visitor) override {
      visitor.visit(*this);
    }

    std::any accept(const visitor::EtaASTBuildVisitor &visitor) const override {
      return visitor.visit(*this);
    }
  };

  struct Definition : public ASTNode {
    types::FirstPassTypeJudgement firstJudgement;
    types::SecondPassTypeJudgement secondJudgement;
  };

  struct Use final : public Definition {
    std::string id;

    explicit Use(std::string id, const size_t line,
                 const size_t charPositionInLine)
      : id(std::move(id)) {
      this->line = line;
      this->charPositionInLine = charPositionInLine;
    }

    std::string toString(int indent) override;

    void accept(visitor::EtaASTAnnotateVisitor &visitor) override {
      visitor.visit(*this);
    }

    std::any accept(const visitor::EtaASTBuildVisitor &visitor) const override {
      return visitor.visit(*this);
    }
  };

  struct RecordDefinition final : public Definition {
    std::string id;
    utils::InsertionOrderedMap<std::string, std::unique_ptr<EtaTypeNode> > fields;

    RecordDefinition(std::string id, utils::InsertionOrderedMap<std::string, std::unique_ptr<EtaTypeNode> > fields,  const size_t line, const size_t charPositionInLine)
      : id(std::move(id)), fields(std::move(fields)) {
      this->line = line;
      this->charPositionInLine = charPositionInLine;
    }

    std::string toString(int indent) override;

    void accept(visitor::EtaASTAnnotateVisitor &visitor) override {
      visitor.visit(*this);
    }

    std::any accept(const visitor::EtaASTBuildVisitor &visitor) const override {
      return visitor.visit(*this);
    }
  };

  struct GlobalVariableDeclaration final : public Definition {

    std::unique_ptr<Declaration> declaration;
    std::unique_ptr<Expression> value; // nullable

    explicit GlobalVariableDeclaration(std::unique_ptr<Declaration> declaration,
                               std::unique_ptr<Expression> value,
                               const size_t line, const size_t charPositionInLine)
      : declaration(std::move(declaration)), value(std::move(value)) {
      this->line = line;
      this->charPositionInLine = charPositionInLine;
    }

    std::string toString(int indent) override;

    void accept(visitor::EtaASTAnnotateVisitor &visitor) override {
      visitor.visit(*this);
    }

    std::any accept(const visitor::EtaASTBuildVisitor &visitor) const override {
      return visitor.visit(*this);
    }
  };

  struct LvalueNode : public ASTNode {
    types::LValueNodeTypeJudgement judgement;
  };

  struct DeclarationLvalueNode final : public LvalueNode {
    std::unique_ptr<Declaration> declaration;

    DeclarationLvalueNode(std::unique_ptr<Declaration> declaration,
                          const size_t line, const size_t charPositionInLine)
      : declaration(std::move(declaration)) {
      this->line = line;
      this->charPositionInLine = charPositionInLine;
    }

    std::string toString(int indent) override;

    void accept(visitor::EtaASTAnnotateVisitor &visitor) override {
      visitor.visit(*this);
    }

    std::any accept(const visitor::EtaASTBuildVisitor &visitor) const override {
      return visitor.visit(*this);
    }
  };

  struct WildcardLvalueNode final : public LvalueNode {
    WildcardLvalueNode(const size_t line, const size_t charPositionInLine) {
      this->line = line;
      this->charPositionInLine = charPositionInLine;
    }

    std::string toString(int indent) override;

    void accept(visitor::EtaASTAnnotateVisitor &visitor) override {
      visitor.visit(*this);
    }

    std::any accept(const visitor::EtaASTBuildVisitor &visitor) const override {
      return visitor.visit(*this);
    }
  };

  struct LvalueIdentifierNode final : public LvalueNode {
    std::string id;

    explicit LvalueIdentifierNode(std::string id, const size_t line,
                                  const size_t charPositionInLine)
      : id(std::move(id)) {
      this->line = line;
      this->charPositionInLine = charPositionInLine;
    }

    std::string toString(int indent) override;

    void accept(visitor::EtaASTAnnotateVisitor &visitor) override {
      visitor.visit(*this);
    }

    std::any accept(const visitor::EtaASTBuildVisitor &visitor) const override {
      return visitor.visit(*this);
    }
  };

  struct LvalueArrayNode final : public LvalueNode {
    std::unique_ptr<BinaryExpression> arrayIndexExpression;

    LvalueArrayNode(std::unique_ptr<BinaryExpression> arrayIndexExpression,
                    const size_t line, const size_t charPositionInLine)
      : arrayIndexExpression(std::move(arrayIndexExpression)) {
      this->line = line;
      this->charPositionInLine = charPositionInLine;
    }

    std::string toString(int indent) override;

    void accept(visitor::EtaASTAnnotateVisitor &visitor) override {
      visitor.visit(*this);
    }

    std::any accept(const visitor::EtaASTBuildVisitor &visitor) const override {
      return visitor.visit(*this);
    }
  };

  struct LvalueRecordNode final : public LvalueNode {
    std::unique_ptr<Expression> record;
    std::string id;
    explicit LvalueRecordNode(std::unique_ptr<Expression> record, std::string id, const size_t line,const size_t charPositionInLine)
      : record(std::move(record)), id(std::move(id)){
      this->line = line;
      this->charPositionInLine = charPositionInLine;
    }

    std::string toString(int indent) override;

    void accept(visitor::EtaASTAnnotateVisitor &visitor) override {
      visitor.visit(*this);
    }

    std::any accept(const visitor::EtaASTBuildVisitor &visitor) const override {
      return visitor.visit(*this);
    }
  };

  struct AssignmentStatement final : public Statement {
    std::vector<std::unique_ptr<LvalueNode> > lvalues;
    std::vector<std::unique_ptr<Expression> > rvalues;

    AssignmentStatement(std::vector<std::unique_ptr<LvalueNode> > lvalues,
                        std::vector<std::unique_ptr<Expression> > rvalues,
                        const size_t line, const size_t charPositionInLine)
      : lvalues(std::move(lvalues)), rvalues(std::move(rvalues)) {
      this->line = line;
      this->charPositionInLine = charPositionInLine;
    }

    std::string toString(int indent) override;

    void accept(visitor::EtaASTAnnotateVisitor &visitor) override {
      visitor.visit(*this);
    }

    std::any accept(const visitor::EtaASTBuildVisitor &visitor) const override {
      return visitor.visit(*this);
    }
  };

  struct MethodInterface final : Definition {

    std::string id;
    std::vector<Declaration> declarations; // parameters
    std::vector<std::unique_ptr<EtaTypeNode> > return_types;

    MethodInterface(std::string id, std::vector<Declaration> declarations,
                    std::vector<std::unique_ptr<EtaTypeNode> > return_types,
                    const size_t line, const size_t charPositionInLine)
      : id(std::move(id)),
        declarations(std::move(declarations)),
        return_types(std::move(return_types)) {
      this->line = line;
      this->charPositionInLine = charPositionInLine;
    }

    std::string toString(int indent) override;

    void accept(visitor::EtaASTAnnotateVisitor &visitor) override {
      visitor.visit(*this);
    }

    std::any accept(const visitor::EtaASTBuildVisitor &visitor) const override {
      return visitor.visit(*this);
    }
  };

  struct Method final : Definition {

    std::unique_ptr<MethodInterface> header;
    std::unique_ptr<BlockStatement> body;

    Method(std::unique_ptr<MethodInterface> header,
           std::unique_ptr<BlockStatement> body, const size_t line,
           const size_t charPositionInLine)
      : header(std::move(header)), body(std::move(body)) {
      this->line = line;
      this->charPositionInLine = charPositionInLine;
    }

    std::string toString(int indent) override;

    void accept(visitor::EtaASTAnnotateVisitor &visitor) override {
      visitor.visit(*this);
    }

    std::any accept(const visitor::EtaASTBuildVisitor &visitor) const override {
      return visitor.visit(*this);
    }
  };

  struct Program final : public ASTNode {
    std::string name;
    types::FirstPassTypeJudgement firstJudgement;
    types::SecondPassTypeJudgement secondJudgement;
    std::vector<std::unique_ptr<Definition> > definitions;

    Program(std::vector<std::unique_ptr<Definition> > definitions,
            const size_t line, const size_t charPositionInLine)
      : definitions(std::move(definitions)) {
      this->line = line;
      this->charPositionInLine = charPositionInLine;
    }

    std::string toString(int indent) override;

    void accept(visitor::EtaASTAnnotateVisitor &visitor) override {
      visitor.visit(*this);
    }

    [[nodiscard]] std::vector<Use> useClauses() const {
      std::vector<Use> useClauses;
      for (const auto &defn : definitions) {
        if (const auto ptr = dynamic_cast<Use *>(defn.get())) {
          useClauses.push_back(*ptr);
        }
      }
      return useClauses;
    }

    std::any accept(const visitor::EtaASTBuildVisitor &visitor) const override {
      return visitor.visit(*this);
    }
  };

  // using InterfaceDefinition = std::variant<MethodInterface, RecordDefinition>;
  // struct Interface final : public ASTNode {
  //   types::FirstPassTypeJudgement judgement;
  //   types::SecondPassTypeJudgement secondJudgement;
  //
  //   std::vector<std::unique_ptr<Use> > use_clauses;
  //   std::vector<std::unique_ptr<Definition> > interface_definitions;
  //
  //   explicit Interface(
  //     std::vector<std::unique_ptr<Definition> > interface_definitions,
  //     const size_t line, const size_t charPositionInLine)
  //     : interface_definitions(std::move(interface_definitions)) {
  //     this->line = line;
  //     this->charPositionInLine = charPositionInLine;
  //   }
  //
  //   std::string toString(int indent) override;
  //
  //   void accept(visitor::EtaASTAnnotateVisitor &visitor) override {
  //     visitor.visit(*this);
  //   }
  //
  //   std::any accept(const visitor::EtaASTBuildVisitor &visitor) const override {
  //     return visitor.visit(*this);
  //   }
  // };
} // namespace ssy38::my_compiler::ast
