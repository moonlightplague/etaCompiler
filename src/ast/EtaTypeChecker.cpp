#include "EtaTypeChecker.hpp"

#include <format>
#include <stdexcept>
#include <string>
#include <vector>

#include "ast.hpp"
#include "utils.hpp"

namespace ssy38::my_compiler::types {
  void FirstPassTypeChecker::visit(ast::Definition &defn) { defn.accept(*this); }

  void FirstPassTypeChecker::visit(ast::GlobalVariableDeclaration &decl) {
    decl.firstJudgement.output = decl.firstJudgement.input;
  }

  void FirstPassTypeChecker::visit(ast::Method &method) {
    method.firstJudgement.output = method.firstJudgement.input;
  }

  void FirstPassTypeChecker::visit(ast::MethodInterface &method_interface) {
    method_interface.firstJudgement.output = method_interface.firstJudgement.input;
  }

  void FirstPassTypeChecker::visit(ast::RecordDefinition &definition) {
    const types::TypeContextEntry value(std::in_place_type<types::PlaceHolder>, nullptr);
    definition.firstJudgement.output = definition.firstJudgement.input.set(
      types::getRecordDefnAccessorStr(definition.id), value);
  }

  void FirstPassTypeChecker::visit(ast::Use &use) {
    if (use.firstJudgement.visited.find(use.id)) {
      throw utils::EtaFrontendError(
          utils::CompilerMode::TypeCheck, use.line,use.charPositionInLine,
          std::format("circular use found for {}", use.id));
    }
    const auto it = interfaces.find(use.id);
    if (it == interfaces.end()) {
      throw utils::EtaFrontendError(
         utils::CompilerMode::TypeCheck, use.line,use.charPositionInLine,
         std::format("interface {} not found", use.id));
    }
    auto*interface = it->second;
    interface->firstJudgement.input = use.firstJudgement.input;
    interface->firstJudgement.visited = interface->firstJudgement.visited.insert(use.id);
    interface->accept(*this);
    use.firstJudgement.output = interface->firstJudgement.output;
  }

  void FirstPassTypeChecker::visit(ast::Program &program) {
    auto gamma = program.firstJudgement.input;

    for (const auto& defn : program.definitions) {
      defn->firstJudgement.input = gamma;
      defn->firstJudgement.visited = program.firstJudgement.visited;
      defn->accept(*this);
      gamma = defn->firstJudgement.output;
    }
    program.firstJudgement.output = gamma;
  }

  void SecondPassTypeChecker::visit(ast::Definition &defn) { defn.accept(*this); }

  void SecondPassTypeChecker::visit(ast::Use &use) {
    auto*interface = interfaces.find(use.id)->second;
    interface->secondJudgement.input = use.secondJudgement.input;
    bool old = isProgram;
    isProgram = false;
    interface->accept(*this);
    isProgram = old;
    use.secondJudgement.output = interface->secondJudgement.output;
  }

  void SecondPassTypeChecker::visit(ast::RecordDefinition &definition) {
    RecordDefnType type;
    MethodType constructorType({}, {});
    OrdinaryTypeAnnotatorVisitor visitor(definition.secondJudgement.input);
    for (const auto &[fieldName, fieldTypeNode] : definition.fields) {
      fieldTypeNode->accept(visitor);
      type.insert({fieldName, fieldTypeNode->ordinaryType});
      constructorType.first.push_back(fieldTypeNode->ordinaryType);
    }
    constructorType.second.push_back(std::make_shared<RecordType>(definition.id));
    if (auto ptr = definition.secondJudgement.input.find(getRecordDefnAccessorStr(definition.id))) {
      if (std::get_if<PlaceHolder>(ptr)) {
        if (definition.secondJudgement.input.find(definition.id)) {
          throw utils::EtaFrontendError(
           utils::CompilerMode::TypeCheck, definition.line, definition.charPositionInLine,
         std::format("definition of the function {} conflicts with constructor of a record type", definition.id));
        }
        if (isProgram) {
          definition.secondJudgement.output =
          definition.secondJudgement.input.set(getRecordDefnAccessorStr(definition.id), type)
                  .set(definition.id, constructorType);
        }
        else {
          definition.secondJudgement.output =
          definition.secondJudgement.input.set(getRecordDefnAccessorStr(definition.id), type);
        }
      }
      else if (auto definedType = std::get_if<RecordDefnType>(ptr)) {
        if (!recordDefnTypesEqual(type, *definedType)) {
          throw utils::EtaFrontendError(
           utils::CompilerMode::TypeCheck, definition.line, definition.charPositionInLine,
         "conflicting definitions of record type " + definition.id);
        }
        definition.secondJudgement.output = definition.secondJudgement.input;
      }
      else {
        throw utils::EtaFrontendError(
          utils::CompilerMode::TypeCheck, definition.line, definition.charPositionInLine,
          "SecondPassTypeChecker::visit(ast::RecordDefinition &definition): impossible branch encountered");
      }
    }
    else {
      throw utils::EtaFrontendError(
          utils::CompilerMode::TypeCheck, definition.line, definition.charPositionInLine,
          "SecondPassTypeChecker::visit(ast::RecordDefinition &definition): impossible branch encountered");
    }
  }

  void SecondPassTypeChecker::visit(ast::Method &method) {
    method.header->secondJudgement.input = method.secondJudgement.input;
    method.header->accept(*this);
    method.secondJudgement.output = method.header->secondJudgement.output;
  }

  void SecondPassTypeChecker::visit(ast::MethodInterface &method_interface) {
    // Populate each parameter type
    OrdinaryTypeAnnotatorVisitor visitor(method_interface.secondJudgement.input);
    TupleType paramTypes;
    for (const auto& [_, paramTypeNode] : method_interface.declarations) {
      paramTypeNode->accept(visitor);
      paramTypes.push_back(paramTypeNode->ordinaryType);
    }

    // Populate each return type
    TupleType returnTypes;
    for (const auto& returnTypeNode : method_interface.return_types) {
      returnTypeNode->accept(visitor);
      returnTypes.push_back(returnTypeNode->ordinaryType);
    }

    const MethodType type(paramTypes, returnTypes);
    if (auto defined = method_interface.secondJudgement.input.find(method_interface.id)) {  // id() already declared
      if (std::get_if<MethodType>(defined) && !methodTypesEqual(*std::get_if<MethodType>(defined),type))
      throw utils::EtaFrontendError(
          utils::CompilerMode::TypeCheck, method_interface.line,
          method_interface.charPositionInLine,
          std::format("{} already declared", method_interface.id));
    }
    method_interface.secondJudgement.output =
        method_interface.secondJudgement.input.set(
            method_interface.id, MethodType(paramTypes, returnTypes));
  }

  void SecondPassTypeChecker::visit(ast::GlobalVariableDeclaration &decl) {
    if (decl.secondJudgement.input.find(decl.declaration->first)) {  // id already declared
      throw utils::EtaFrontendError(utils::CompilerMode::TypeCheck, decl.line,
                                    decl.charPositionInLine,
                                    std::format("{} already declared", decl.declaration->first));
    }

    // Annotate and get decl's (ordinary) type
    OrdinaryTypeAnnotatorVisitor visitor(decl.secondJudgement.input);
    decl.declaration->second->accept(visitor);
    std::shared_ptr<OrdinaryType> type = decl.declaration->second->ordinaryType;

    decl.secondJudgement.output = decl.secondJudgement.input.set(
        decl.declaration->first, TypeContextEntry(VariableType(VariableCategory::Global, type)));
  }

  void SecondPassTypeChecker::visit(ast::Program &program) {
    auto gamma = program.secondJudgement.input;
    for (const auto& defn : program.definitions) {
      defn->secondJudgement.input = gamma;
      defn->accept(*this);
      gamma = defn->secondJudgement.output;
    }
    program.secondJudgement.output = gamma;
  }

  // Expressions
void ThirdPassTypeChecker::visit(ast::Expression& expr) { expr.accept(*this); }

void ThirdPassTypeChecker::visit(ast::UnaryExpression& expr) {
  expr.expr->judgement.input = expr.judgement.input;
  expr.expr->accept(*this);
  auto intType = std::make_shared<IntType>();
  auto boolType = std::make_shared<BoolType>();
  auto* asInt =
      dynamic_cast<IntType*>(greater(expr.expr->judgement.type, intType).get());
  auto* asBool = dynamic_cast<BoolType*>(
      greater(expr.expr->judgement.type, boolType).get());
  std::string opstr = ast::UnaryOperand2String(expr.op);
  if (expr.op == ast::Expression::UnaryOperand::Minus) {
    if (!asInt) {
      throw utils::EtaFrontendError(
          utils::CompilerMode::TypeCheck, expr.line, expr.charPositionInLine,
          std::format("Operand of {} must be int.", opstr));
    }
    expr.judgement.type = std::make_shared<IntType>();
  }
  if (expr.op == ast::Expression::UnaryOperand::Bang) {
    if (!asBool) {
      throw utils::EtaFrontendError(
          utils::CompilerMode::TypeCheck, expr.line, expr.charPositionInLine,
          std::format("Operand of {} must be bool.", opstr));
    }
    expr.judgement.type = std::make_shared<BoolType>();
  }
}

void ThirdPassTypeChecker::visit(ast::BinaryExpression& expr) {
  expr.left->judgement.input = expr.judgement.input;
  expr.right->judgement.input = expr.judgement.input;
  expr.left->accept(*this);
  expr.right->accept(*this);
  auto intType = std::make_shared<IntType>();
  auto boolType = std::make_shared<BoolType>();
  auto anyArrayType =
      std::make_shared<ArrayType>(std::make_shared<NothingType>());
  auto* asIntLeft =
      dynamic_cast<IntType*>(greater(expr.left->judgement.type, intType).get());
  auto* asIntRight = dynamic_cast<IntType*>(
      greater(expr.right->judgement.type, intType).get());
  auto* asBoolLeft = dynamic_cast<BoolType*>(
      greater(expr.left->judgement.type, boolType).get());
  auto* asBoolRight = dynamic_cast<BoolType*>(
      greater(expr.right->judgement.type, boolType).get());
  auto* asArrayLeft = dynamic_cast<ArrayType*>(
      greater(expr.left->judgement.type, anyArrayType).get());
  auto* asArrayRight = dynamic_cast<ArrayType*>(
      greater(expr.right->judgement.type, anyArrayType).get());
  std::string opstr = ast::BinaryOperand2String(expr.op);

  if (expr.op == ast::Expression::BinaryOperand::Equal || expr.op == ast::Expression::BinaryOperand::NotEqual) {
    if (!expr.right->judgement.type->leq(*expr.left->judgement.type) &&
      !expr.left->judgement.type->leq(*expr.right->judgement.type)) {
      throw utils::EtaFrontendError(
          utils::CompilerMode::TypeCheck, expr.line, expr.charPositionInLine,
          "Incompatible types used in equality/inequality operator.");
    }
    expr.judgement.type = std::make_shared<BoolType>();
  }
  else if (asIntLeft && asIntRight &&
      (expr.op == ast::Expression::BinaryOperand::Plus ||
       expr.op == ast::Expression::BinaryOperand::Minus ||
       expr.op == ast::Expression::BinaryOperand::Multiply ||
       expr.op == ast::Expression::BinaryOperand::HighMultiply ||
       expr.op == ast::Expression::BinaryOperand::Divide ||
       expr.op == ast::Expression::BinaryOperand::Mod)) {
    expr.judgement.type = std::make_shared<IntType>();
  } else if (asIntLeft && asIntRight &&
             (expr.op == ast::Expression::BinaryOperand::Equal ||
              expr.op == ast::Expression::BinaryOperand::NotEqual ||
              expr.op == ast::Expression::BinaryOperand::LessThan ||
              expr.op == ast::Expression::BinaryOperand::LessThanEqual ||
              expr.op == ast::Expression::BinaryOperand::GreaterThan ||
              expr.op == ast::Expression::BinaryOperand::GreaterThanEqual)) {
    expr.judgement.type = std::make_shared<BoolType>();
  } else if (asBoolLeft && asBoolRight &&
             (expr.op == ast::Expression::BinaryOperand::Equal ||
              expr.op == ast::Expression::BinaryOperand::NotEqual ||
              expr.op == ast::Expression::BinaryOperand::And ||
              expr.op == ast::Expression::BinaryOperand::Or)) {
    expr.judgement.type = std::make_shared<BoolType>();
  } else if (asArrayLeft && asArrayRight &&
             (expr.op == ast::Expression::BinaryOperand::Equal ||
              expr.op == ast::Expression::BinaryOperand::NotEqual)) {
    if (!greater(asArrayLeft->subType, asArrayRight->subType)) {
      throw utils::EtaFrontendError(
          utils::CompilerMode::TypeCheck, expr.line, expr.charPositionInLine,
          "Subtypes inside the array between two operands are not compatible.");
    }
    expr.judgement.type = std::make_shared<BoolType>();
  } else if (asArrayLeft && asIntRight &&
             expr.op == ast::Expression::BinaryOperand::ArrayAccess) {
    expr.judgement.type = asArrayLeft->subType;
  } else if (asArrayLeft && asArrayRight &&
             expr.op == ast::Expression::BinaryOperand::Plus) {
    if (!greater(asArrayLeft->subType, asArrayRight->subType)) {
      throw utils::EtaFrontendError(
          utils::CompilerMode::TypeCheck, expr.line, expr.charPositionInLine,
          "Array types between two operands are not compatible.");
    }
    expr.judgement.type = std::make_shared<ArrayType>(
        greater(asArrayLeft->subType, asArrayRight->subType));
  } else {
    throw utils::EtaFrontendError(
        utils::CompilerMode::TypeCheck, expr.line, expr.charPositionInLine,
        std::format("Unexpected type for binary operation {}: {}, {}", opstr,
                    expr.left->judgement.type->toString(),
                    expr.right->judgement.type->toString()));
  }
}

void ThirdPassTypeChecker::visit(ast::ArrayInitializerExpression& expr) {
  if (expr.exprs.empty()) {
    expr.judgement.type =
        std::make_shared<ArrayType>(std::make_shared<NothingType>());
    return;
  }
  expr.exprs[0]->judgement.input = expr.judgement.input;
  expr.exprs[0]->accept(*this);
  auto ptr = expr.exprs[0]->judgement.type;
  for (std::size_t i = 0; i < expr.exprs.size() - 1; i++) {
    expr.exprs[i + 1]->judgement.input = expr.judgement.input;
    expr.exprs[i + 1]->accept(*this);
    ptr = greater(ptr, expr.exprs[i + 1]->judgement.type);
    if (!ptr) {
      throw utils::EtaFrontendError(
          utils::CompilerMode::TypeCheck, expr.line, expr.charPositionInLine,
          "All the elements in the array must have the same type.");
    }
  }
  expr.judgement.type = std::make_shared<ArrayType>(ptr);
}

void ThirdPassTypeChecker::visit(ast::FunctionApplicationExpression& expr) {
  const auto it = expr.judgement.input.find(expr.id);
  if (!it) {
    throw utils::EtaFrontendError(
        utils::CompilerMode::TypeCheck, expr.line, expr.charPositionInLine,
        std::format("Name {} cannot be resolved.", expr.id));
  }
  auto* p = std::get_if<MethodType>(it);
  if (!p) {
    throw utils::EtaFrontendError(
        utils::CompilerMode::TypeCheck, expr.line, expr.charPositionInLine,
        std::format("{} is not a function.", expr.id));
  }
  auto paramLength = p->first.size();
  auto exprlength = expr.exprs.size();
  auto returnLength = p->second.size();
  if (returnLength != 1) {
    throw utils::EtaFrontendError(
        utils::CompilerMode::TypeCheck, expr.line, expr.charPositionInLine,
        std::format("Expected 1 output type but gets {}", returnLength));
  }
  if (paramLength != exprlength) {
    throw utils::EtaFrontendError(
        utils::CompilerMode::TypeCheck, expr.line, expr.charPositionInLine,
        std::format("Mismatched number of input values in function {}; "
                    "required {} but get {}.",
                    expr.id, paramLength, exprlength));
  }
  for (const auto& arg : expr.exprs) {
    arg->judgement.input = expr.judgement.input;
    arg->accept(*this);
  }
  for (std::size_t i = 0; i < paramLength; i++) {
    if (!expr.exprs[i]->judgement.type->leq(*(p->first[i]))) {
      throw utils::EtaFrontendError(
          utils::CompilerMode::TypeCheck, expr.exprs[i]->line,
          expr.exprs[i]->charPositionInLine,
          std::format("Mismatched type in function input."));
    }
  }
  expr.judgement.type = p->second[0];
}

void ThirdPassTypeChecker::visit(ast::LengthExpression& expr) {
  expr.expr->judgement.input = expr.judgement.input;
  expr.expr->accept(*this);
  if (!dynamic_cast<ArrayType*>(expr.expr->judgement.type.get()) &&
      !dynamic_cast<NothingType*>(expr.expr->judgement.type.get())) {
    throw utils::EtaFrontendError(utils::CompilerMode::TypeCheck,
                                  expr.expr->line,
                                  expr.expr->charPositionInLine,
                                  "Length function only accepts array type.");
  }
  expr.judgement.type = std::make_shared<IntType>();
}

void ThirdPassTypeChecker::visit(ast::IdentifierExpression& expr) {
  const auto it = expr.judgement.input.find(expr.id);
  if (!it) {
    throw utils::EtaFrontendError(
        utils::CompilerMode::TypeCheck, expr.line, expr.charPositionInLine,
        std::format("Name {} cannot be resolved.", expr.id));
  }
  auto* p = std::get_if<VariableType>(it);
  if (!p) {
    throw utils::EtaFrontendError(
        utils::CompilerMode::TypeCheck, expr.line, expr.charPositionInLine,
        std::format("{} is not a variable.", expr.id));
  }
  expr.judgement.type = p->second;
}

void ThirdPassTypeChecker::visit(ast::BoolLiteralExpression& expr) {
  expr.judgement.type = std::make_shared<BoolType>();
}

void ThirdPassTypeChecker::visit(ast::IntLiteralExpression& expr) {
  expr.judgement.type = std::make_shared<IntType>();
}

void ThirdPassTypeChecker::visit(ast::StringLiteralExpression& expr) {
  expr.judgement.type =
      std::make_shared<ArrayType>(std::make_shared<IntType>());
}

void ThirdPassTypeChecker::visit(ast::CharLiteralExpression& expr) {
  expr.judgement.type = std::make_shared<IntType>();
}

void ThirdPassTypeChecker::visit(ast::NullLiteralExpression &expr) {
  expr.judgement.type = std::make_shared<NullType>();
}

void ThirdPassTypeChecker::visit(ast::FieldAccessExpression &expr) {
    expr.expr->judgement.input = expr.judgement.input;
    expr.expr->accept(*this);
    auto type = expr.expr->judgement.type;
    if (dynamic_cast<NullType *>(type.get())) {
      expr.judgement.type = std::make_shared<NothingType>();
    }
    else if (dynamic_cast<NothingType *>(type.get())) {
      expr.judgement.type = std::make_shared<NothingType>();
    }
    else if (const auto recordType = dynamic_cast<RecordType * const>(type.get())) {
      if (auto recordDefn =
          std::get_if<RecordDefnType>(expr.judgement.input.find(getRecordDefnAccessorStr(recordType->name)))) {
        auto it = recordDefn->find(expr.id);
        if (it == recordDefn->end()) {
          throw utils::EtaFrontendError(utils::CompilerMode::TypeCheck, expr.line, expr.charPositionInLine,
         std::format("invalid field {} for record type {}", expr.id, recordType->name));
        }
        expr.judgement.type = it->second;
      }
      else
        throw std::runtime_error("ThirdPassTypeChecker::visit(ast::FieldAccessExpression &expr): impossible branch encountered");
    }
    else {
      throw utils::EtaFrontendError(utils::CompilerMode::TypeCheck, expr.line, expr.charPositionInLine,
         "cannot access field of a non-record type");
    }
}

// Statements
void ThirdPassTypeChecker::visit(ast::Statement& stmt) { stmt.accept(*this); }

void ThirdPassTypeChecker::visit(ast::ReturnStatement& stmt) {
  // fetch return type
  auto returnType = stmt.judgement.input.find(RETURN_TYPE_KEY);
  if (!returnType) {
    throw std::runtime_error(
        "EtaTypeChecker::visit: return type doesn't exist. This shouldn't "
        "happen if we implemented things right.");
  }

  // get return type; throw if return type is not a tuple type
  if (auto* typeListPtr = std::get_if<ReturnType>(returnType)) {
    // check if the return expression list is the same length as function
    // signature
    if (typeListPtr->size() != stmt.exprs.size()) {
      throw utils::EtaFrontendError(
          utils::CompilerMode::TypeCheck, stmt.line, stmt.charPositionInLine,
          "number of return values supplied does not match function signature");
    }

    // derive the type for each expression inside the expression list
    for (int i = 0; i < stmt.exprs.size(); i++) {
      auto& expr = stmt.exprs[i];
      auto& type = (*typeListPtr)[i];
      expr->judgement.input = stmt.judgement.input;
      expr->accept(*this);
      // if type doesn't match, error
      if (!expr->judgement.type->leq(*type)) {
        throw utils::EtaFrontendError(utils::CompilerMode::TypeCheck,
                                      expr->line, expr->charPositionInLine,
                                      "return type mismatch");
      }
    }

    // assign output typing context and statement type
    stmt.judgement.output = stmt.judgement.input;
    stmt.judgement.type = StatementType::Void;
  } else {
    throw std::runtime_error(
        "EtaTypeChecker::visit: return type is not a tuple type. Someone "
        "implemented things wrong. ");
  }
}

void ThirdPassTypeChecker::visit(ast::BreakStatement &stmt) {
    if (!stmt.judgement.input.find(BREAK_TYPE_KEY)) {
      throw utils::EtaFrontendError(utils::CompilerMode::TypeCheck,
                                        stmt.line, stmt.charPositionInLine,
                                        "Dangling break statement");
    }
    stmt.judgement.output = stmt.judgement.input;
    stmt.judgement.type = StatementType::Void;
}

void ThirdPassTypeChecker::visit(ast::IfStatement& stmt) {
  // check condition has type bool
  stmt.condition->judgement.input = stmt.judgement.input;
  stmt.condition->accept(*this);
  auto boolean = BoolType();
  if (!stmt.condition->judgement.type->leq(boolean)) {
    throw utils::EtaFrontendError(
        utils::CompilerMode::TypeCheck, stmt.condition->line,
        stmt.condition->charPositionInLine,
        "guard of if statement does not have type bool");
  }

  // derive type of first statement
  stmt.then_branch->judgement.input = stmt.judgement.input;
  stmt.then_branch->accept(*this);

  // if else branch is null, the if statement has return type unit directly
  if (stmt.else_branch == nullptr) {
    stmt.judgement.type = StatementType::Unit;
  }
  // else, derive type of second statement and compute lub
  else {
    stmt.else_branch->judgement.input = stmt.judgement.input;
    stmt.else_branch->accept(*this);
    stmt.judgement.type =
        stmt.then_branch->judgement.type == StatementType::Void &&
                stmt.else_branch->judgement.type == StatementType::Void
            ? StatementType::Void
            : StatementType::Unit;
  }

  // for both cases the output context is the input context
  stmt.judgement.output = stmt.judgement.input;
}

void ThirdPassTypeChecker::visit(ast::WhileStatement& stmt) {
  // check condition has type bool
  stmt.condition->judgement.input = stmt.judgement.input;
  stmt.condition->accept(*this);
  auto boolean = BoolType();
  if (!stmt.condition->judgement.type->leq(boolean)) {
    throw utils::EtaFrontendError(
        utils::CompilerMode::TypeCheck, stmt.condition->line,
        stmt.condition->charPositionInLine,
        "guard of while statement does not have type bool");
  }

  // derive type of body
  stmt.body->judgement.input = stmt.judgement.input.set(BREAK_TYPE_KEY, static_cast<PlaceHolder>(nullptr));
  stmt.body->accept(*this);
  // compute statement type and output context
  stmt.judgement.type = StatementType::Unit;
  stmt.judgement.output = stmt.judgement.input;
}

void ThirdPassTypeChecker::visit(ast::BlockStatement& stmt) {
  // if block is empty, the statement has type unit
  if (stmt.statements.empty()) {
    stmt.judgement.type = StatementType::Unit;
  }
  // otherwise derive the types of the statements in order
  else {
    types::GammaType currentContext = stmt.judgement.input;
    for (int i = 0; i < stmt.statements.size() - 1; i++) {
      auto& currentStatement = stmt.statements[i];
      currentStatement->judgement.input = currentContext;
      currentStatement->accept(*this);
      // all but the last statement in a block cannot have void type
      if (currentStatement->judgement.type == StatementType::Void) {
        throw utils::EtaFrontendError(utils::CompilerMode::TypeCheck,
                                      currentStatement->line,
                                      currentStatement->charPositionInLine,
                                      "a statement that is not the last "
                                      "statement in its block has void type");
      }
      currentContext = currentStatement->judgement.output;
    }
    // derive the last statement, and extract its return type
    auto& lastStatement = stmt.statements.back();
    lastStatement->judgement.input = currentContext;
    lastStatement->accept(*this);
    stmt.judgement.type = lastStatement->judgement.type;
  }
  // don't forget to return the output context
  stmt.judgement.output = stmt.judgement.input;
}

types::MethodType getFunctionSignature(std::string id, GammaType gamma,
                                       size_t line, size_t charPositionInLine) {
  auto funTypeEntry = gamma.find(id);
  if (!funTypeEntry) {
    throw utils::EtaFrontendError(utils::CompilerMode::TypeCheck, line,
                                  charPositionInLine,
                                  std::format("method {} does not exist", id));
  }
  if (auto* funTypePtr = std::get_if<MethodType>(funTypeEntry)) {
    return *funTypePtr;
  } else {
    throw utils::EtaFrontendError(
        utils::CompilerMode::TypeCheck, line, charPositionInLine,
        std::format("identifier {} is not a method", id));
  }
};

void checkFunctionInput(
    ThirdPassTypeChecker& checker, const GammaType& input,
    std::vector<std::unique_ptr<ast::Expression>>& arguments,
    const TupleType& types, size_t line, size_t charPositionInLine) {
  if (arguments.size() != types.size()) {
    throw utils::EtaFrontendError(
        utils::CompilerMode::TypeCheck, line, charPositionInLine,
        "function is applied to an incorrect number of parameters");
  }
  for (int i = 0; i < arguments.size(); i++) {
    auto& type = types[i];
    auto& arg = arguments[i];
    arg->judgement.input = input;
    arg->accept(checker);
    if (!arg->judgement.type->leq(*type)) {
      throw utils::EtaFrontendError(
          utils::CompilerMode::TypeCheck, line, charPositionInLine,
          std::format("procedure requires type {} for argument #{}, got type "
                      "{} instead",
                      type->toString(), i + 1,
                      arg->judgement.type->toString()));
    }
  }
}

void ThirdPassTypeChecker::visit(ast::ProcedureCall& stmt) {
  const auto& [input_types, output_types] = getFunctionSignature(
      stmt.id, stmt.judgement.input, stmt.line, stmt.charPositionInLine);
  if (!output_types.empty()) {
    throw utils::EtaFrontendError(
        utils::CompilerMode::TypeCheck, stmt.line, stmt.charPositionInLine,
        std::format("function {} is not a procedure", stmt.id));
  }
  checkFunctionInput(*this, stmt.judgement.input, stmt.arguments, input_types,
                     stmt.line, stmt.charPositionInLine);
  stmt.judgement.type = StatementType::Unit;
  stmt.judgement.output = stmt.judgement.input;
}

void ThirdPassTypeChecker::visit(ast::DeclarationStatement& stmt) {
  GammaType currentContext = stmt.judgement.input;
  OrdinaryTypeAnnotatorVisitor visitor(stmt.judgement.input);
  for (const auto& [name, typeNode] : stmt.declarations) {
    typeNode->accept(visitor);
    typeNode->input = stmt.judgement.input;
    typeNode->accept(ArrayTypeIndexInDeclarationVisitor::getInstance());
    if (currentContext.find(name)) {
      throw utils::EtaFrontendError(
          utils::CompilerMode::TypeCheck, typeNode->line,
          typeNode->charPositionInLine, "redeclared variable");
    }
    currentContext = currentContext.set(
        name, VariableType(VariableCategory::Local, typeNode->ordinaryType));
  }
  stmt.judgement.type = StatementType::Unit;
  stmt.judgement.output = currentContext;
}

void ThirdPassTypeChecker::visit(ast::AssignmentStatement& stmt) {
  // derive the types of lvalues using the helper type checker
  auto currentContext = stmt.judgement.input;
  for (auto& lvalue : stmt.lvalues) {
    lvalue->judgement.input = stmt.judgement.input;
    lvalue->judgement.current = currentContext;
    lvalue->accept(LValueTypeChecker::getInstance());
    currentContext = lvalue->judgement.output;
  }
  // choose which typing rule to use. If there's only one rvalue and contains
  // only one function application expression, enter rule MultiAssignCall
  if (stmt.rvalues.size() == 1 &&
      dynamic_cast<ast::FunctionApplicationExpression*>(
          stmt.rvalues.front().get()) != nullptr) {
    auto* funPtr = dynamic_cast<ast::FunctionApplicationExpression*>(
        stmt.rvalues.front().get());
    // fetch type of function
    const auto& [input_types, output_types] =
        getFunctionSignature(funPtr->id, stmt.judgement.input, funPtr->line,
                             funPtr->charPositionInLine);
    // check function application is well-typed
    checkFunctionInput(*this, stmt.judgement.input, funPtr->exprs, input_types,
                       funPtr->line, funPtr->charPositionInLine);
    // check output types are well-typed
    if (stmt.lvalues.size() != output_types.size()) {
      throw utils::EtaFrontendError(
          utils::CompilerMode::TypeCheck, stmt.line, stmt.charPositionInLine,
          "cannot unpack -- return type of function call does not match the "
          "number of rvalues");
    }
    for (int i = 0; i < stmt.lvalues.size(); i++) {
      if (!output_types[i]->leq(*(stmt.lvalues[i]->judgement.type))) {
        throw utils::EtaFrontendError(utils::CompilerMode::TypeCheck,
                                      stmt.lvalues[i]->line,
                                      stmt.lvalues[i]->charPositionInLine,
                                      "cannot unpack -- return type of "
                                      "function and lvalue type doesn't match");
      }
    }
  }
  // otherwise, enter rule MultiAssign
  else {
    if (stmt.rvalues.size() != stmt.lvalues.size()) {
      throw utils::EtaFrontendError(
          utils::CompilerMode::TypeCheck, stmt.line, stmt.charPositionInLine,
          "number of lvalues does not match the number of rvalues");
    }
    // derive the type of rvalues
    for (int i = 0; i < stmt.rvalues.size(); i++) {
      auto& rvalue = stmt.rvalues[i];
      auto& lvalue = stmt.lvalues[i];
      rvalue->judgement.input = stmt.judgement.input;
      rvalue->accept(*this);
      if (!rvalue->judgement.type->leq(*(lvalue->judgement.type))) {
        throw utils::EtaFrontendError(
            utils::CompilerMode::TypeCheck, stmt.lvalues[i]->line,
            stmt.lvalues[i]->charPositionInLine,
            "type of lvalue and rvalue does not match");
      }
    }
  }
  stmt.judgement.type = StatementType::Unit;
  stmt.judgement.output = currentContext;
}

void ThirdPassTypeChecker::visit(ast::Definition &defn) { defn.accept(*this); }

// Declarations
void ThirdPassTypeChecker::visit(ast::GlobalVariableDeclaration& decl) {
  const std::string id = decl.declaration->first;
      // ordinary type was annotated by second pass
  const std::shared_ptr<OrdinaryType> type = decl.declaration->second->ordinaryType;
  // Check against value's type if there is a value
  if (decl.value != nullptr) {
    decl.value->judgement.input = input;
    decl.value->accept(*this);
    if (std::shared_ptr<OrdinaryType> valueType = decl.value->judgement.type; !valueType->leq(*type)) {
      throw utils::EtaFrontendError(
          utils::CompilerMode::TypeCheck, decl.value->line,
          decl.value->charPositionInLine,
          std::format("{} cannot be assigned to {}", valueType->toString(),
                      id));
    }
  }
}

void ThirdPassTypeChecker::visit(ast::MethodInterface& method_interface) {
      // check each param id to ensure it doesn't shadow an existing param id or
      // a top-level variable id
      std::set<std::string> params;
      for (const auto& [paramId, _] : method_interface.declarations) {
        if (params.contains(paramId)) {
          throw utils::EtaFrontendError(
              utils::CompilerMode::TypeCheck, method_interface.line,
              method_interface.charPositionInLine,
              std::format("parameter {} shadows an existing parameter",
                          paramId));
        }
        params.insert(paramId);
        if (input.find(paramId)) {
          throw utils::EtaFrontendError(
              utils::CompilerMode::TypeCheck, method_interface.line,
              method_interface.charPositionInLine,
              std::format("parameter {} shadows an existing variable/function",
                          paramId));
        }
      }
      output = output.set(method_interface.id, *input.find(method_interface.id));
}

void ThirdPassTypeChecker::visit(ast::Method& method) {
      // visit header to check parameter shadowing in second pass
      method.header->accept(*this);

      // initialize method body's type context with params and return type
      auto bodyGamma = input;
      for (const auto& [paramId, paramTypeNode] : method.header->declarations) {
        // paramTypeNode->ordinaryType already visited and annotated by
        // OrdinaryTypeAnnotatorVisitor in first pass
        bodyGamma = bodyGamma.set(
            paramId,
            VariableType(VariableCategory::Local, paramTypeNode->ordinaryType));
      }
      const auto methodType = std::get_if<MethodType>(input.find(method.header->id));
      bodyGamma =
          bodyGamma.set(RETURN_TYPE_KEY, methodType->second);
      method.body->judgement.input = bodyGamma;
      // visit body
      method.body->accept(*this);

      // ensure a function with return values has a body with a void
      // StatementType
      if (!methodType->second.empty() &&
          method.body->judgement.type == StatementType::Unit) {
        throw utils::EtaFrontendError(
            utils::CompilerMode::TypeCheck, method.line,
            method.charPositionInLine,
            std::format("function {} missing return statement",
                        method.header->id));
      }
      output = output.set(method.header->id, *methodType);
}

void ThirdPassTypeChecker::visit(ast::Use &use) {}

void ThirdPassTypeChecker::visit(ast::RecordDefinition &definition) {
  output = output.set(getRecordDefnAccessorStr(definition.id), *input.find(getRecordDefnAccessorStr(definition.id)));
}


// Top-level
void ThirdPassTypeChecker::visit(ast::Program& program) {
  for (const auto& defn : program.definitions) {
    defn->accept(*this);
  }
}

void OrdinaryTypeAnnotatorVisitor::visit(ast::EtaTypeNode& node) {
  node.accept(*this);
}

void OrdinaryTypeAnnotatorVisitor::visit(ast::BaseTypeNode& node) {
  switch (node.type) {
    case ast::EtaTypeNode::BaseType::Integer:
      node.ordinaryType = std::make_shared<IntType>();
      break;
    case ast::EtaTypeNode::BaseType::Boolean:
      node.ordinaryType = std::make_shared<BoolType>();
      break;
  }
}

void OrdinaryTypeAnnotatorVisitor::visit(ast::RecordTypeNode &node) {
  if (input.find(getRecordDefnAccessorStr(node.name))) {
    node.ordinaryType = std::make_shared<RecordType>(node.name);
  }
  else {
    throw utils::EtaFrontendError(utils::CompilerMode::TypeCheck, node.line,
          node.charPositionInLine,
          "record type " + node.name + " not found");
  }
}

void OrdinaryTypeAnnotatorVisitor::visit(ast::ArrayTypeNode& node) {
  node.subtype->accept(*this);
  node.ordinaryType = std::make_shared<ArrayType>(node.subtype->ordinaryType);
}

void ArrayTypeIndexInDeclarationVisitor::visit(ast::EtaTypeNode& node) {
  node.accept(*this);
}

void ArrayTypeIndexInDeclarationVisitor::visit(ast::BaseTypeNode& node) {
  // nothing to check
}

void ArrayTypeIndexInDeclarationVisitor::visit(ast::RecordTypeNode &node) {
  //nothing to check
}

void ArrayTypeIndexInDeclarationVisitor::visit(ast::ArrayTypeNode& node) {
  // check the index expression is well-typed
  node.subtype->input = node.input;
  if (node.dimension) {
    auto checker = ThirdPassTypeChecker(node.input);
    node.dimension->judgement.input = node.input;
    node.dimension->accept(checker);
    IntType intType{};
    if (!node.dimension->judgement.type->leq(intType)) {
      throw utils::EtaFrontendError(
          utils::CompilerMode::TypeCheck, node.dimension->line,
          node.dimension->charPositionInLine,
          "array index in initialization does not have type int");
    }
    // visit the subtype
    node.subtype->input = node.input;
    node.subtype->accept(*this);
  }
}

void LValueTypeChecker::visit(ast::LvalueNode& node) { node.accept(*this); }

void LValueTypeChecker::visit(ast::LvalueIdentifierNode& node) {
  // get type of variable
  auto typePtr = node.judgement.input.find(node.id);
  if (!typePtr) {
    throw utils::EtaFrontendError(
        utils::CompilerMode::TypeCheck, node.line, node.charPositionInLine,
        std::format("Attempting to assign variable {} before declaration",
                    node.id));
  }

  // fetch type of variable
  if (auto* varTypePtr = std::get_if<VariableType>(typePtr)) {
    node.judgement.type = varTypePtr->second;
    node.judgement.output = node.judgement.current;
  } else {
    throw utils::EtaFrontendError(
        utils::CompilerMode::TypeCheck, node.line, node.charPositionInLine,
        std::format("identifier {} is not a variable", node.id));
  }
}

void LValueTypeChecker::visit(ast::LvalueRecordNode &node) {
  node.record->judgement.input = node.judgement.input;
  auto checker = ThirdPassTypeChecker(node.judgement.input);
  node.record->accept(checker);
  auto type = node.record->judgement.type;
  if (dynamic_cast<NullType *>(type.get())) {
    node.judgement.type = std::make_shared<NothingType>();
  }
  else if (dynamic_cast<NothingType *>(type.get())) {
    node.judgement.type = std::make_shared<NothingType>();
  }
  else if (const auto recordType = dynamic_cast<RecordType * const>(type.get())) {
    if (auto recordDefn =
        std::get_if<RecordDefnType>(node.judgement.input.find(getRecordDefnAccessorStr(recordType->name)))) {
      auto it = recordDefn->find(node.id);
      if (it == recordDefn->end()) {
        throw utils::EtaFrontendError(utils::CompilerMode::TypeCheck, node.line, node.charPositionInLine,
       std::format("invalid field {} for record type {}", node.id, recordType->name));
      }
      node.judgement.type = it->second;
        }
    else
      throw std::runtime_error("ThirdPassTypeChecker::visit(ast::FieldAccessExpression &expr): impossible branch encountered");
  }
  else {
    throw utils::EtaFrontendError(utils::CompilerMode::TypeCheck, node.line, node.charPositionInLine,
       "cannot access field of a non-record type");
  }
  node.judgement.output = node.judgement.current;
}

void LValueTypeChecker::visit(ast::WildcardLvalueNode& node) {
  // wildcards have the special type that equals to any other type during type
  // checking
  node.judgement.type = std::make_shared<types::AnyType>();
  node.judgement.output = node.judgement.current;
}

void LValueTypeChecker::visit(ast::DeclarationLvalueNode& node) {
  // check if the identifier has already been declared
  auto& [name, typeNode] = *node.declaration;
  if (node.judgement.current.find(name)) {
    throw utils::EtaFrontendError(utils::CompilerMode::TypeCheck, node.line,
                                  node.charPositionInLine,
                                  "redeclared variable");
  }
  // visit the type of the declaration
  OrdinaryTypeAnnotatorVisitor visitor(node.judgement.input);
  node.declaration->second->accept(visitor);
  node.judgement.type = typeNode->ordinaryType;
  node.judgement.output = node.judgement.current.set(
      name, VariableType(VariableCategory::Local, node.judgement.type));
}

void LValueTypeChecker::visit(ast::LvalueArrayNode& node) {
  // typecheck the array index expression
  node.arrayIndexExpression->judgement.input = node.judgement.input;
  //this checker doesn't need any interfaces, so the code below is ok
  auto checker = ThirdPassTypeChecker(node.judgement.input);
  node.arrayIndexExpression->accept(checker);
  node.judgement.type = node.arrayIndexExpression->judgement.type;
  node.judgement.output = node.judgement.current;
}
}  // namespace ssy38::my_compiler::types
