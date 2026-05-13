#pragma once

#include <ast.hpp>
#include <ranges>

#include "generated/etaParseBaseVisitor.h"
#include "utils.hpp"

namespace ast = ssy38::my_compiler::ast;

/**
 * A visitor that transforms an ANTLR4 generated parse tree into an AST
 */
class EtaParseTreeVisitor : public etaParseBaseVisitor {
 public:
  /**
   * Constructs an etaASTVisitor
   *
   * @param stream An ANTLR4 token stream, which is needed to ensure that a
   * statement cannot be preceded by illegal characters
   */
  explicit EtaParseTreeVisitor(antlr4::TokenStream *stream)
      : _tokenStream(stream) {}

  std::any visitUseClause(etaParse::UseClauseContext *context) override {
    return static_cast<ast::Definition *>(
        new ast::Use(context->ID()->getText(), context->getStart()->getLine(),
                     context->getStart()->getCharPositionInLine() + 1));
  }

  std::any visitNullAtom(etaParse::NullAtomContext *context) override {
      return static_cast<ast::Expression *>(new ast::NullLiteralExpression(
          context->getStart()->getLine(),context->getStart()->getCharPositionInLine() + 1));
  }

  std::any visitIntegerAtom(etaParse::IntegerAtomContext *context) override {
    const auto token = context->getStart();
    const int64_t value = parseTokenToInt64(
        context->negative == nullptr, context->INTEGER()->getText(),
        token->getLine(), token->getCharPositionInLine());
    return static_cast<ast::Expression *>(new ast::IntLiteralExpression(
        value, token->getLine(), token->getCharPositionInLine() + 1));
  }

  std::any visitTrueAtom(etaParse::TrueAtomContext *context) override {
    return static_cast<ast::Expression *>(new ast::BoolLiteralExpression(
        true, context->getStart()->getLine(),
        context->getStart()->getCharPositionInLine() + 1));
  }

  std::any visitFalseAtom(etaParse::FalseAtomContext *context) override {
    return static_cast<ast::Expression *>(new ast::BoolLiteralExpression(
        false, context->getStart()->getLine(),
        context->getStart()->getCharPositionInLine() + 1));
  }

  std::any visitStringAtom(etaParse::StringAtomContext *context) override {
    // remove quotation marks
    const std::string cleaned = context->STRING()->getText().substr(
        1, context->STRING()->getText().size() - 2);
    try {
      const auto &[escaped, codepoints] = ssy38::utils::escape_string(cleaned);
      return static_cast<ast::Expression *>(new ast::StringLiteralExpression(
          escaped, codepoints, context->getStart()->getLine(),
          context->getStart()->getCharPositionInLine() + 1));
    } catch (const std::invalid_argument &e) {
      throw ssy38::utils::EtaFrontendError(
          ssy38::utils::CompilerMode::Parse, context->getStart()->getLine(),
          context->getStart()->getCharPositionInLine() + 1, e.what());
    }
  }

  std::any visitCharacterAtom(
      etaParse::CharacterAtomContext *context) override {
    // remove quotation marks
    const std::string cleaned = context->CHARACTER()->getText().substr(
        1, context->CHARACTER()->getText().size() - 2);
    try {
      const auto &[escaped, codepoints] = ssy38::utils::escape_string(cleaned);
      return static_cast<ast::Expression *>(new ast::CharLiteralExpression(
          escaped, codepoints[0], context->getStart()->getLine(),
          context->getStart()->getCharPositionInLine() + 1));
    } catch (const std::invalid_argument &e) {
      throw ssy38::utils::EtaFrontendError(
          ssy38::utils::CompilerMode::Parse, context->getStart()->getLine(),
          context->getStart()->getCharPositionInLine() + 1, e.what());
    }
  }

  std::any visitIdentifierAtom(
      etaParse::IdentifierAtomContext *context) override {
    return static_cast<ast::Expression *>(new ast::IdentifierExpression(
        context->ID()->getText(), context->getStart()->getLine(),
        context->getStart()->getCharPositionInLine() + 1));
  }

  std::any visitIfStatement(etaParse::IfStatementContext *context) override {
    auto condition = std::unique_ptr<ast::Expression>(
        std::any_cast<ast::Expression *>(visit(context->expression())));
    auto true_branch = std::unique_ptr<ast::Statement>(
        std::any_cast<ast::Statement *>(visit(context->trueBranch)));
    std::unique_ptr<ast::Statement> false_branch = nullptr;
    if (context->falseBranch != nullptr) {
      false_branch = std::unique_ptr<ast::Statement>(
          std::any_cast<ast::Statement *>(visit(context->falseBranch)));
    }
    return static_cast<ast::Statement *>(new ast::IfStatement(
        std::move(condition), std::move(true_branch), std::move(false_branch),
        context->getStart()->getLine(),
        context->getStart()->getCharPositionInLine() + 1));
  }

  std::any visitBreakStatement(etaParse::BreakStatementContext *context) override {
      return static_cast<ast::Statement *>(new ast::BreakStatement(
      context->getStart()->getLine(),
    context->getStart()->getCharPositionInLine() + 1));
  }

  std::any visitWhileStatement(
      etaParse::WhileStatementContext *context) override {
    auto condition = std::unique_ptr<ast::Expression>(
        std::any_cast<ast::Expression *>(visit(context->expression())));
    auto body = std::unique_ptr<ast::Statement>(
        std::any_cast<ast::Statement *>(visit(context->statement())));
    return static_cast<ast::Statement *>(new ast::WhileStatement(
        std::move(condition), std::move(body), context->getStart()->getLine(),
        context->getStart()->getCharPositionInLine() + 1));
  }

  std::any visitUnaryExpression(
      etaParse::UnaryExpressionContext *context) override {
    ast::Expression::UnaryOperand operand;
    if (context->op->getText() == "-")
      operand = ast::Expression::UnaryOperand::Minus;
    else if (context->op->getText() == "!")
      operand = ast::Expression::UnaryOperand::Bang;
    else
      throw ssy38::utils::EtaFrontendError(
          ssy38::utils::CompilerMode::Parse, context->op->getLine(),
          context->op->getCharPositionInLine() + 1, "invalid unary expression");

    auto expr = std::unique_ptr<ast::Expression>(
        std::any_cast<ast::Expression *>(visit(context->expression())));
    return static_cast<ast::Expression *>(new ast::UnaryExpression(
        operand, std::move(expr), context->getStart()->getLine(),
        context->getStart()->getCharPositionInLine() + 1));
  }

  std::any visitMultiplicativeExpression(
      etaParse::MultiplicativeExpressionContext *context) override {
    return visitArithmeticBinaryExpression(context->op, context->expression(0),
                                           context->expression(1));
  }

  std::any visitAdditiveExpression(
      etaParse::AdditiveExpressionContext *context) override {
    return visitArithmeticBinaryExpression(context->op, context->expression(0),
                                           context->expression(1));
  }

  std::any visitInequalityExpression(
      etaParse::InequalityExpressionContext *context) override {
    return visitArithmeticBinaryExpression(context->op, context->expression(0),
                                           context->expression(1));
  }

  std::any visitEqualityExpression(
      etaParse::EqualityExpressionContext *context) override {
    return visitArithmeticBinaryExpression(context->op, context->expression(0),
                                           context->expression(1));
  }

  std::any visitLogicalAndExpression(
      etaParse::LogicalAndExpressionContext *context) override {
    return visitArithmeticBinaryExpression(context->op, context->expression(0),
                                           context->expression(1));
  }

  std::any visitLogicalOrExpession(
      etaParse::LogicalOrExpessionContext *context) override {
    return visitArithmeticBinaryExpression(context->op, context->expression(0),
                                           context->expression(1));
  }

  std::any visitArrayAccessExpression(
      etaParse::ArrayAccessExpressionContext *context) override {
    auto left = std::unique_ptr<ast::Expression>(
        std::any_cast<ast::Expression *>(visit(context->expression(0))));
    auto right = std::unique_ptr<ast::Expression>(
        std::any_cast<ast::Expression *>(visit(context->expression(1))));
    return static_cast<ast::Expression *>(new ast::BinaryExpression(
        ast::Expression::BinaryOperand::ArrayAccess, std::move(left),
        std::move(right), context->getStart()->getLine(),
        context->getStart()->getCharPositionInLine() + 1));
  }

  std::any visitFieldAccessExpression(etaParse::FieldAccessExpressionContext *context) override {
      auto expr = std::unique_ptr<ast::Expression>(
          std::any_cast<ast::Expression *>(visit(context->expression())));
      auto id = context->ID()->getText();
      return static_cast<ast::Expression *>(new ast::FieldAccessExpression(std::move(expr), id,
          context->getStart()->getLine(),context->getStart()->getCharPositionInLine() + 1));
  }

  std::any visitLengthExpression(
      etaParse::LengthExpressionContext *ctx) override {
    auto expr = std::unique_ptr<ast::Expression>(
        std::any_cast<ast::Expression *>(visit(ctx->expression())));
    return static_cast<ast::Expression *>(new ast::LengthExpression(
        std::move(expr), ctx->getStart()->getLine(),
        ctx->getStart()->getCharPositionInLine() + 1));
  }

  std::any visitParenthesisExpression(
      etaParse::ParenthesisExpressionContext *context) override {
    return static_cast<ast::Expression *>(
        std::any_cast<ast::Expression *>(visit(context->expression())));
  }

  std::any visitNonemptyExpressionList(
      etaParse::NonemptyExpressionListContext *context) override {
    auto *expressions = new std::vector<std::unique_ptr<ast::Expression>>();
    expressions->reserve(context->items.size());
    for (const auto item : context->items) {
      expressions->emplace_back(std::any_cast<ast::Expression *>(visit(item)));
    }
    return static_cast<std::vector<std::unique_ptr<ast::Expression>> *>(
        expressions);
  }

  std::any visitExpressionList(
      etaParse::ExpressionListContext *context) override {
    if (context->nonemptyExpressionList() == nullptr) {
      return new std::vector<std::unique_ptr<ast::Expression>>();
    }
    return static_cast<std::vector<std::unique_ptr<ast::Expression>> *>(
        std::any_cast<std::vector<std::unique_ptr<ast::Expression>> *>(
            visit(context->nonemptyExpressionList())));
  }

  std::any visitReturnStatement(
      etaParse::ReturnStatementContext *context) override {
    const auto expressions_ptr =
        std::unique_ptr<std::vector<std::unique_ptr<ast::Expression>>>(
            std::any_cast<std::vector<std::unique_ptr<ast::Expression>> *>(
                visit(context->expressionList())));
    return static_cast<ast::Statement *>(new ast::ReturnStatement(
        std::move(*expressions_ptr), context->getStart()->getLine(),
        context->getStart()->getCharPositionInLine() + 1));
  }

  std::any visitArrayInitializerExpression(
      etaParse::ArrayInitializerExpressionContext *context) override {
    const auto expressions_ptr =
        std::unique_ptr<std::vector<std::unique_ptr<ast::Expression>>>(
            std::any_cast<std::vector<std::unique_ptr<ast::Expression>> *>(
                visit(context->expressionList())));
    return static_cast<ast::Expression *>(new ast::ArrayInitializerExpression(
        std::move(*expressions_ptr), context->getStart()->getLine(),
        context->getStart()->getCharPositionInLine() + 1));
  }

  std::any visitProcedureCallStatement(
      etaParse::ProcedureCallStatementContext *context) override {
    const auto expressions_ptr =
        std::unique_ptr<std::vector<std::unique_ptr<ast::Expression>>>(
            std::any_cast<std::vector<std::unique_ptr<ast::Expression>> *>(
                visit(context->expressionList())));
    return static_cast<ast::Statement *>(new ast::ProcedureCall(
        context->ID()->getText(), std::move(*expressions_ptr),
        context->getStart()->getLine(),
        context->getStart()->getCharPositionInLine() + 1));
  }

  std::any visitFunctionApplicationExpression(
      etaParse::FunctionApplicationExpressionContext *context) override {
    const auto expressions_ptr =
        std::unique_ptr<std::vector<std::unique_ptr<ast::Expression>>>(
            std::any_cast<std::vector<std::unique_ptr<ast::Expression>> *>(
                visit(context->expressionList())));
    const std::string id = context->ID()->getText();
    ;
    return static_cast<ast::Expression *>(
        new ast::FunctionApplicationExpression(
            id, std::move(*expressions_ptr), context->getStart()->getLine(),
            context->getStart()->getCharPositionInLine() + 1));
  }

std::any visitIntBaseType(etaParse::IntBaseTypeContext *context) override {
      return static_cast<ast::EtaTypeNode *>(
          new ast::BaseTypeNode(ast::EtaTypeNode::BaseType::Integer, context->getStart()->getLine(),
              context->getStart()->getCharPositionInLine() + 1));
  }

 std::any visitBoolBaseType(etaParse::BoolBaseTypeContext *context) override {
      return static_cast<ast::EtaTypeNode *>(
          new ast::BaseTypeNode(ast::EtaTypeNode::BaseType::Boolean, context->getStart()->getLine(),
              context->getStart()->getCharPositionInLine() + 1));
  }

 std::any visitRecordBaseType(etaParse::RecordBaseTypeContext *context) override {
      auto id = context->recordTypeSpecifier()->ID()->getText();
      return static_cast<ast::EtaTypeNode *>(new ast::RecordTypeNode(id,
          context->getStart()->getLine(), context->getStart()->getCharPositionInLine() + 1));
  }

  std::any visitTypeSpecifierInAssignmentOrMethod(
      etaParse::TypeSpecifierInAssignmentOrMethodContext *context) override {
    // basically just a linked list
    auto baseType = std::unique_ptr<ast::EtaTypeNode>(
        std::any_cast<ast::EtaTypeNode *>(visit(context->baseType())));
    auto head = createEtaTypeNodeLinkedListWithNoDimensions(
        std::move(baseType), context->brackets.size(),
        context->getStart()->getLine(),
        context->getStart()->getCharPositionInLine());
    return static_cast<ast::EtaTypeNode *>(head.release());
  }

  std::any visitTypeSpecifierInDeclaration(
      etaParse::TypeSpecifierInDeclarationContext *context) override {
    // basically just a linked list
      auto baseType = std::unique_ptr<ast::EtaTypeNode>(
          std::any_cast<ast::EtaTypeNode *>(visit(context->baseType())));
    auto head = createEtaTypeNodeLinkedListWithNoDimensions(
        std::move(baseType), context->brackets.size(),
        context->getStart()->getLine(),
        context->getStart()->getCharPositionInLine());

    // add brackets with dimensions
    for (const auto dimensionExpressionContext :
         std::ranges::reverse_view(context->indexes)) {
      auto dim = std::unique_ptr<ast::Expression>(
          std::any_cast<ast::Expression *>(visit(dimensionExpressionContext)));
      std::unique_ptr<ast::EtaTypeNode> new_head =
          std::make_unique<ast::ArrayTypeNode>(
              std::move(head), std::move(dim), context->getStart()->getLine(),
              context->getStart()->getCharPositionInLine() + 1);
      head = std::move(new_head);
    }

    return static_cast<ast::EtaTypeNode *>(head.release());
  }

  std::any visitBlockStatement(
      etaParse::BlockStatementContext *context) override {
    auto statements = std::vector<std::unique_ptr<ast::Statement>>();
    statements.reserve(context->statementList()->items.size() + 1);

    for (const auto item : context->statementList()->items) {
      statements.emplace_back(std::any_cast<ast::Statement *>(visit(item)));
    }
    if (context->returnClause != nullptr) {
      statements.emplace_back(
          std::any_cast<ast::Statement *>(visit(context->returnClause)));
    }
    return static_cast<ast::Statement *>(new ast::BlockStatement(
        std::move(statements), context->getStart()->getLine(),
        context->getStart()->getCharPositionInLine() + 1));
  }

  std::any visitDeclaration(etaParse::DeclarationContext *context) override {
    auto typeSpecifierPtr =
        std::unique_ptr<ast::EtaTypeNode>(std::any_cast<ast::EtaTypeNode *>(
            visit(context->typeSpecifierInDeclaration())));
    return static_cast<ast::Declaration *>(new ast::Declaration(
        context->ID()->getText(), std::move(typeSpecifierPtr)));
  }

  std::any visitEtaDeclarationStatement(
      etaParse::EtaDeclarationStatementContext *context) override {
    auto declarations = std::vector<ast::Declaration>();
    declarations.reserve(context->items.size());

    for (const auto item : context->items) {
      auto idTypePair_ptr = std::unique_ptr<ast::Declaration>(
          std::any_cast<ast::Declaration *>(visit(item)));
      declarations.push_back(std::move(*idTypePair_ptr));
    }
    return static_cast<ast::Statement *>(new ast::DeclarationStatement(
        std::move(declarations), context->getStart()->getLine(),
        context->getStart()->getCharPositionInLine() + 1));
  }

  std::any visitRhoDeclarationStatement(etaParse::RhoDeclarationStatementContext *context) override {
      auto ids = context->sameTypeDeclarationList()->items;
      auto declarations = std::vector<ast::Declaration>();
      declarations.reserve(ids.size());
      for (const auto id : ids) {
          auto type = std::unique_ptr<ast::EtaTypeNode>(
         std::any_cast<ast::EtaTypeNode *>(visit(context->sameTypeDeclarationList()->typeSpecifierInAssignmentOrMethod())));
          declarations.emplace_back(id->getText(), std::move(type));
      }
      return static_cast<ast::Statement *>(new ast::DeclarationStatement(
        std::move(declarations), context->getStart()->getLine(),
        context->getStart()->getCharPositionInLine() + 1));
  }

  std::any visitDeclarationLvalue(
      etaParse::DeclarationLvalueContext *context) override {
    auto typeSpecifierPtr =
        std::unique_ptr<ast::EtaTypeNode>(std::any_cast<ast::EtaTypeNode *>(
            visit(context->typeSpecifierInAssignmentOrMethod())));

    auto declaration = std::make_unique<ast::Declaration>(ast::Declaration(
        context->ID()->getText(), std::move(typeSpecifierPtr)));
    return static_cast<ast::LvalueNode *>(new ast::DeclarationLvalueNode(
        std::move(declaration), context->getStart()->getLine(),
        context->getStart()->getCharPositionInLine() + 1));
  }

  std::any visitAssignmentIdLvalue(
      etaParse::AssignmentIdLvalueContext *ctx) override {
    return static_cast<ast::LvalueNode *>(new ast::LvalueIdentifierNode(
        ctx->ID()->getText(), ctx->getStart()->getLine(),
        ctx->getStart()->getCharPositionInLine() + 1));
  }

  std::any visitArrayCellAssignment(
      etaParse::ArrayCellAssignmentContext *ctx) override {
    auto left = std::unique_ptr<ast::Expression>(
        std::any_cast<ast::Expression *>(visit(ctx->expression(0))));
    auto right = std::unique_ptr<ast::Expression>(
        std::any_cast<ast::Expression *>(visit(ctx->expression(1))));
    auto arrayIndexExpPtr = std::make_unique<ast::BinaryExpression>(
        ast::Expression::BinaryOperand::ArrayAccess, std::move(left),
        std::move(right), ctx->getStart()->getLine(),
        ctx->getStart()->getCharPositionInLine() + 1);
    return static_cast<ast::LvalueNode *>(new ast::LvalueArrayNode(
        std::move(arrayIndexExpPtr), ctx->getStart()->getLine(),
        ctx->getStart()->getCharPositionInLine() + 1));
  }

  std::any visitFieldAccessLvalue(etaParse::FieldAccessLvalueContext *context) override {
      auto expr = std::unique_ptr<ast::Expression>(
            std::any_cast<ast::Expression *>(visit(context->expression())));
      auto id = context->ID()->getText();
      return static_cast<ast::LvalueNode *>(new ast::LvalueRecordNode(std::move(expr), id,
          context->getStart()->getLine(),context->getStart()->getCharPositionInLine() + 1));
  }
  //  std::any visitAssignmentLvalue(
  //      etaParse::AssignmentLvalueContext *context) override {
  //    std::unique_ptr<ast::AssignmentLvalueNode> head =
  //        std::make_unique<ast::LvalueIdentifierNode>(context->ID()->getText());
  //
  //    for (const auto i : context->index) {
  //      auto expr = std::unique_ptr<ast::Expression>(
  //          std::any_cast<ast::Expression *>(visit(i)));
  //      std::unique_ptr<ast::AssignmentLvalueNode> new_head =
  //          std::make_unique<ast::LvalueArrayNode>(std::move(head),
  //                                                 std::move(expr));
  //      head = std::move(new_head);
  //    }
  //
  //    return static_cast<ast::LvalueNode *>(new
  //    ast::LvalueNode(std::move(head)));
  //  }

  std::any visitWildcardLvalue(
      etaParse::WildcardLvalueContext *context) override {
    return static_cast<ast::LvalueNode *>(new ast::WildcardLvalueNode(
        context->getStart()->getLine(),
        context->getStart()->getCharPositionInLine() + 1));
  }

  std::any visitLvalueList(etaParse::LvalueListContext *context) override {
    auto *lvalues = new std::vector<std::unique_ptr<ast::LvalueNode>>();
    lvalues->reserve(context->items.size());
    for (const auto item : context->items) {
      lvalues->emplace_back(std::any_cast<ast::LvalueNode *>(visit(item)));
    }
    return static_cast<std::vector<std::unique_ptr<ast::LvalueNode>> *>(
        lvalues);
  }

  std::any visitAssignmentStatement(
      etaParse::AssignmentStatementContext *context) override {
    const auto token = context->getStart();
    const size_t startIndex = token->getTokenIndex();
    const auto prevToken = _tokenStream->get(startIndex)->getText();
    if (prevToken == "(" || prevToken == "{" || prevToken == "\"") {
      throw ssy38::utils::EtaFrontendError(
          ssy38::utils::CompilerMode::Parse, token->getLine(),
          token->getCharPositionInLine() + 1,
          " error:invalid character preceding assignment statement");
    }
    const auto lvalues =
        std::unique_ptr<std::vector<std::unique_ptr<ast::LvalueNode>>>(
            std::any_cast<std::vector<std::unique_ptr<ast::LvalueNode>> *>(
                visit(context->lvalueList())));
    const auto rvalues =
        std::unique_ptr<std::vector<std::unique_ptr<ast::Expression>>>(
            std::any_cast<std::vector<std::unique_ptr<ast::Expression>> *>(
                visit(context->nonemptyExpressionList())));
    return static_cast<ast::Statement *>(new ast::AssignmentStatement(
        std::move(*lvalues), std::move(*rvalues), token->getLine(),
        token->getCharPositionInLine() + 1));
  }

  std::any visitNonemptyMethodTypeList(
      etaParse::NonemptyMethodTypeListContext *context) override {
    auto *types = new std::vector<std::unique_ptr<ast::EtaTypeNode>>();
    types->reserve(context->items.size());
    for (const auto item : context->items) {
      types->emplace_back(std::any_cast<ast::EtaTypeNode *>(visit(item)));
    }
    return static_cast<std::vector<std::unique_ptr<ast::EtaTypeNode>> *>(types);
  }

  std::any visitMethodParamList(
      etaParse::MethodParamListContext *context) override {
    const auto declarations = new std::vector<ast::Declaration>();
    for (int i = 0; i < context->paramNames.size(); i++) {
      auto id = context->paramNames[i]->getText();
      auto typeSpecifier = std::unique_ptr<ast::EtaTypeNode>(
          std::any_cast<ast::EtaTypeNode *>(visit(context->paramTypes[i])));
      declarations->emplace_back(std::move(id), std::move(typeSpecifier));
    }
    return static_cast<std::vector<ast::Declaration> *>(declarations);
  }

  std::any visitMethodDeclaration(
      etaParse::MethodDeclarationContext *context) override {
    const auto declaration = std::unique_ptr<std::vector<ast::Declaration>>(
        std::any_cast<std::vector<ast::Declaration> *>(
            visit(context->methodParamList())));
    std::unique_ptr<std::vector<std::unique_ptr<ast::EtaTypeNode>>>
        return_types;
    if (context->returnTypes == nullptr) {
      return_types =
          std::make_unique<std::vector<std::unique_ptr<ast::EtaTypeNode>>>();
    } else {
      return_types =
          std::unique_ptr<std::vector<std::unique_ptr<ast::EtaTypeNode>>>(
              std::any_cast<std::vector<std::unique_ptr<ast::EtaTypeNode>> *>(
                  visit(context->returnTypes)));
    }
    return static_cast<ast::Definition *>(new ast::MethodInterface(
        context->ID()->getText(), std::move(*declaration),
        std::move(*return_types), context->getStart()->getLine(),
        context->getStart()->getCharPositionInLine() + 1));
  }

  std::any visitMethodDefinition(
      etaParse::MethodDefinitionContext *context) override {
    auto header = std::unique_ptr<ast::MethodInterface>(
        dynamic_cast<ast::MethodInterface *>(
        std::any_cast<ast::Definition *>(
            visit(context->methodDeclaration()))));
    // visit() returns a Statement*, so we need to dynamic cast (downcast)
    auto block = std::unique_ptr<ast::BlockStatement>(
        dynamic_cast<ast::BlockStatement *>(
            std::any_cast<ast::Statement *>(visit(context->blockStatement()))));
    return static_cast<ast::Definition *>(new ast::Method(
        std::move(header), std::move(block), context->getStart()->getLine(),
        context->getStart()->getCharPositionInLine() + 1));
  }

  std::any visitGlobalNegativeInteger(
      etaParse::GlobalNegativeIntegerContext *context) override {
    const auto token = context->getStart();
    const int64_t value =
        parseTokenToInt64(false, context->INTEGER()->getText(),
                          token->getLine(), token->getCharPositionInLine());
    return static_cast<ast::Expression *>(new ast::IntLiteralExpression(
        value, token->getLine(), token->getCharPositionInLine() + 1));
  }

  std::any visitGlobalPositiveInteger(
      etaParse::GlobalPositiveIntegerContext *context) override {
    const auto token = context->getStart();
    const int64_t value =
        parseTokenToInt64(true, context->INTEGER()->getText(), token->getLine(),
                          token->getCharPositionInLine());
    return static_cast<ast::Expression *>(new ast::IntLiteralExpression(
        value, token->getLine(), token->getCharPositionInLine() + 1));
  }

  std::any visitGlobalCharacter(
      etaParse::GlobalCharacterContext *context) override {
    const std::string cleaned = context->CHARACTER()->getText().substr(
        1, context->CHARACTER()->getText().size() - 2);
    try {
      const auto &[escaped, codepoints] = ssy38::utils::escape_string(cleaned);
      return static_cast<ast::Expression *>(new ast::CharLiteralExpression(
          escaped, codepoints[0], context->getStart()->getLine(),
          context->getStart()->getCharPositionInLine() + 1));
    } catch (const std::invalid_argument &e) {
      throw ssy38::utils::EtaFrontendError(
          ssy38::utils::CompilerMode::Parse, context->getStart()->getLine(),
          context->getStart()->getCharPositionInLine() + 1, e.what());
    }
  }

  std::any visitGlobalTrue(etaParse::GlobalTrueContext *context) override {
    return static_cast<ast::Expression *>(new ast::BoolLiteralExpression(
        true, context->getStart()->getLine(),
        context->getStart()->getCharPositionInLine() + 1));
  }

  std::any visitGlobalFalse(etaParse::GlobalFalseContext *context) override {
    return static_cast<ast::Expression *>(new ast::BoolLiteralExpression(
        false, context->getStart()->getLine(),
        context->getStart()->getCharPositionInLine() + 1));
  }

  std::any visitEtaGlobalDeclaration(
      etaParse::EtaGlobalDeclarationContext *context) override {
    auto typeSpecifier =
        std::unique_ptr<ast::EtaTypeNode>(std::any_cast<ast::EtaTypeNode *>(
            visit(context->typeSpecifierInAssignmentOrMethod())));
    std::unique_ptr<ast::Expression> value;
    if (context->initialized == nullptr) {
      value = nullptr;
    } else {
      value = std::unique_ptr<ast::Expression>(
          std::any_cast<ast::Expression *>(visit(context->initialized)));
    }
    auto ptr =
        std::make_unique<ast::GlobalVariableDeclaration>(
            std::make_unique<ast::Declaration>(context->ID()->getText(),
                                               std::move(typeSpecifier)),
            std::move(value), context->getStart()->getLine(),
            context->getStart()->getCharPositionInLine() + 1);

      auto* vec = new std::vector<std::unique_ptr<ast::Definition>>();
      vec->emplace_back(std::move(ptr));
      return vec;
  }
  //    auto declaration = std::unique_ptr<ast::Declaration>(
  //        std::any_cast<ast::Declaration *>(visit(context->declaration())));
  //    std::unique_ptr<ast::Expression> value;
  //    if (context->initialized == nullptr) {
  //      value = nullptr;
  //    } else {
  //      value = std::unique_ptr<ast::Expression>(
  //          std::any_cast<ast::Expression *>(visit(context->initialized)));
  //    }
  //    return static_cast<ast::Definition *>(new ast::Definition(
  //        ast::GlobalDeclaration(std::move(declaration), std::move(value))));
  //  }
  std::any visitRhoGlobalDeclaration(etaParse::RhoGlobalDeclarationContext *context) override {
      auto ids = context->sameTypeDeclarationList()->items;
      auto* vec = new std::vector<std::unique_ptr<ast::Definition>>();
      for (const auto &id : ids) {
          auto type = std::unique_ptr<ast::EtaTypeNode>(
          std::any_cast<ast::EtaTypeNode *>(visit(context->sameTypeDeclarationList()->typeSpecifierInAssignmentOrMethod())));
          auto ptr =  std::make_unique<ast::GlobalVariableDeclaration>(
            std::make_unique<ast::Declaration>(id->getText(),
                                               std::move(type)),
                                               nullptr,
                                               context->getStart()->getLine(),context->getStart()->getCharPositionInLine() + 1);
          vec->emplace_back(std::move(ptr));
      }
      return vec;
  }

  std::any visitParsingUnit(etaParse::ParsingUnitContext *context) override {
      auto definitions = std::vector<std::unique_ptr<ast::Definition>>();
   definitions.reserve(context->useClauses.size() + context->definitions.size());
    for (const auto clause : context->useClauses) {
      definitions.emplace_back(std::any_cast<ast::Definition *>(visit(clause)));
    }
    definitions.reserve(context->definitions.size());
    for (const auto definition : context->definitions) {
      if (definition->globalDeclaration() == nullptr) {
          definitions.emplace_back(
          std::any_cast<ast::Definition *>(visit(definition)));
      }
      else {
          auto vec = std::move(*std::any_cast<std::vector<std::unique_ptr<ast::Definition>> *>(visit(definition)));
          definitions.insert(
        definitions.end(),
            std::make_move_iterator(vec.begin()),
            std::make_move_iterator(vec.end()));
      }
    }

    return static_cast<ast::Program *>(
        new ast::Program(std::move(definitions),
                         context->getStart()->getLine(),
                         context->getStart()->getCharPositionInLine() + 1));
  }

  std::any visitRecordDefinition(etaParse::RecordDefinitionContext *context) override {
      auto typeName = context->ID()->getText();
      ssy38::utils::InsertionOrderedMap<std::string, std::unique_ptr<ast::EtaTypeNode> > fields;
      for (const auto declaration : context->declarationLists) {
          for (const auto fieldName : declaration->items) {
              auto type = std::unique_ptr<ast::EtaTypeNode>(std::any_cast<ast::EtaTypeNode *>(
                  visit(declaration->typeSpecifierInAssignmentOrMethod())));
              if (fields.contains(fieldName->getText())) {
                  throw ssy38::utils::EtaFrontendError(ssy38::utils::CompilerMode::Parse,
                      fieldName->getLine(), fieldName->getCharPositionInLine()+1,
                      "Re-declaration of field " + fieldName->getText() + " for the record type " + typeName);
              }
              fields.insert({fieldName->getText(), std::move(type)});
          }
      }
      return static_cast<ast::Definition *>(
          new ast::RecordDefinition(typeName, std::move(fields),
            context->getStart()->getLine(), context->getStart()->getCharPositionInLine() + 1));
  }

  // std::any visitInterface(etaParse::InterfaceContext *context) override {
  //   auto methodInterfaces =
  //       std::vector<std::unique_ptr<ast::Definition>>();
  //   methodInterfaces.reserve(context->declarations.size());
  //   for (const auto declaration : context->declarations) {
  //     methodInterfaces.emplace_back(
  //         std::unique_ptr<ast::Definition>(
  //             std::any_cast<ast::Definition *>(visit(declaration))));
  //   }
  //   return static_cast<ast::Interface *>(new ast::Interface(
  //       std::move(methodInterfaces), context->getStart()->getLine(),
  //       context->getStart()->getCharPositionInLine() + 1));
  // }

  std::any visitProgram(etaParse::ProgramContext *context) override {
      auto program = std::unique_ptr<ast::Program>(std::any_cast<ast::Program *>(visit(context->parsingUnit())));
      for (const auto& definition : program->definitions) {
          if (const auto* interface = dynamic_cast<const ast::MethodInterface *>(definition.get())) {
              throw ssy38::utils::EtaFrontendError(ssy38::utils::CompilerMode::Parse, interface->line, interface->charPositionInLine,
              "missing definition for method in program");
          }
      }
      return program.release();
  }

  std::any visitInterface(etaParse::InterfaceContext *context) override {
      auto program = std::unique_ptr<ast::Program>(std::any_cast<ast::Program *>(visit(context->parsingUnit())));
      for (const auto& definition : program->definitions) {
          if (!dynamic_cast<const ast::MethodInterface *>(definition.get())) {
              throw ssy38::utils::EtaFrontendError(ssy38::utils::CompilerMode::Parse, 0, 1,
              "only method headers allowed in Eta interface files");
          }
      }
      return program.release();
  }

    std::any visitRhoInterface(etaParse::RhoInterfaceContext *context) override {
      auto program = std::unique_ptr<ast::Program>(std::any_cast<ast::Program *>(visit(context->parsingUnit())));
      for (const auto& definition : program->definitions) {
          if (const auto* decl = dynamic_cast<const ast::GlobalVariableDeclaration *>(definition.get())) {
              throw ssy38::utils::EtaFrontendError(ssy38::utils::CompilerMode::Parse, decl->line, decl->charPositionInLine,
              "cannot declare global variables in Rho interface files");
          }
          if (const auto* decl = dynamic_cast<const ast::Method *>(definition.get())) {
              throw ssy38::utils::EtaFrontendError(ssy38::utils::CompilerMode::Parse, decl->line, decl->charPositionInLine,
              "cannot define functions in Rho interface files");
          }
      }
      return program.release();
  }

 private:
  antlr4::TokenStream *_tokenStream;

  static ast::Expression::BinaryOperand parseArithmeticBinaryOperand(
      const std::string &token, const size_t tokenLine,
      const size_t tokenPosition) {
    if (token == "*") return ast::Expression::BinaryOperand::Multiply;
    if (token == "*>>") return ast::Expression::BinaryOperand::HighMultiply;
    if (token == "/") return ast::Expression::BinaryOperand::Divide;
    if (token == "%") return ast::Expression::BinaryOperand::Mod;
    if (token == "+") return ast::Expression::BinaryOperand::Plus;
    if (token == "-") return ast::Expression::BinaryOperand::Minus;
    if (token == "<") return ast::Expression::BinaryOperand::LessThan;
    if (token == "<=") return ast::Expression::BinaryOperand::LessThanEqual;
    if (token == ">") return ast::Expression::BinaryOperand::GreaterThan;
    if (token == ">=") return ast::Expression::BinaryOperand::GreaterThanEqual;
    if (token == "==") return ast::Expression::BinaryOperand::Equal;
    if (token == "!=") return ast::Expression::BinaryOperand::NotEqual;
    if (token == "&") return ast::Expression::BinaryOperand::And;
    if (token == "|") return ast::Expression::BinaryOperand::Or;
    throw ssy38::utils::EtaFrontendError(
        ssy38::utils::CompilerMode::Parse, tokenLine, tokenPosition + 1,
        "invalid token encountered at parseArithmeticBinaryOperand: " + token);
  }

  ast::Expression *visitArithmeticBinaryExpression(
      const antlr4::Token *operand_token,
      etaParse::ExpressionContext *left_expression_context,
      etaParse::ExpressionContext *right_expression_context) {
    const ast::Expression::BinaryOperand operand = parseArithmeticBinaryOperand(
        operand_token->getText(), operand_token->getLine(),
        operand_token->getCharPositionInLine());
    auto left = std::unique_ptr<ast::Expression>(
        std::any_cast<ast::Expression *>(visit(left_expression_context)));
    auto right = std::unique_ptr<ast::Expression>(
        std::any_cast<ast::Expression *>(visit(right_expression_context)));
    return static_cast<ast::Expression *>(new ast::BinaryExpression(
        operand, std::move(left), std::move(right), operand_token->getLine(),
        operand_token->getCharPositionInLine() + 1));
  }

  static std::unique_ptr<ast::EtaTypeNode>
  createEtaTypeNodeLinkedListWithNoDimensions(std::unique_ptr<ast::EtaTypeNode> node,
                                              const size_t bracketsSize,
                                              const size_t line,
                                              const size_t charPositionInLine) {
    std::unique_ptr<ast::EtaTypeNode> head = std::move(node);
    for (size_t i = 0; i < bracketsSize; i++) {
      std::unique_ptr<ast::EtaTypeNode> new_head =
          std::make_unique<ast::ArrayTypeNode>(std::move(head), nullptr, line,
                                               charPositionInLine + 1);
      head = std::move(new_head);
    }

    return head;
  }

  static int64_t parseTokenToInt64(const bool isPositive,
                                   const std::string &token,
                                   const size_t tokenLine,
                                   const size_t tokenPosition) {
    try {
      const unsigned long long value = std::stoull(token);
      if (isPositive) {
        if (value > (unsigned long long)INT64_MAX) {
          throw ssy38::utils::EtaFrontendError(
              ssy38::utils::CompilerMode::Parse, tokenLine, tokenPosition + 1,
              "integer literal out of range");
        }
        return static_cast<int64_t>(value);
      } else {
        unsigned long long max_magnitude = (unsigned long long)INT64_MAX + 1ULL;
        if (value > max_magnitude) {
          throw ssy38::utils::EtaFrontendError(
              ssy38::utils::CompilerMode::Parse, tokenLine, tokenPosition + 1,
              "integer literal out of range");
        }
        if (value == max_magnitude) {
          return INT64_MIN;
        } else {
          return -static_cast<int64_t>(value);
        }
      }
    } catch (const std::out_of_range &) {
      throw ssy38::utils::EtaFrontendError(ssy38::utils::CompilerMode::Parse,
                                           tokenLine, tokenPosition + 1,
                                           "integer literal out of range");
    }
  }
};
