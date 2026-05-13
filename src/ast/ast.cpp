#include "ast.hpp"

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "utils.hpp"

namespace ssy38::my_compiler::ast {
std::string Use::toString(int indent) { return "(use " + id + ")"; }

std::string UnaryOperand2String(Expression::UnaryOperand op) {
  using U = Expression::UnaryOperand;
  switch (op) {
    case U::Minus:
      return "-";
    case U::Bang:
      return "!";
  }
  return "<unknown-unary-op>";
}

std::string BinaryOperand2String(Expression::BinaryOperand op) {
  using B = Expression::BinaryOperand;
  switch (op) {
    case B::Multiply:
      return "*";
    case B::HighMultiply:
      return "*>>";
    case B::Divide:
      return "/";
    case B::Mod:
      return "%";
    case B::Plus:
      return "+";
    case B::Minus:
      return "-";
    case B::LessThan:
      return "<";
    case B::LessThanEqual:
      return "<=";
    case B::GreaterThan:
      return ">";
    case B::GreaterThanEqual:
      return ">=";
    case B::Equal:
      return "==";
    case B::NotEqual:
      return "!=";
    case B::And:
      return "&";
    case B::Or:
      return "|";
    case B::ArrayAccess:
      return "[]";
  }
  return "<unknown-binary-op>";
}

std::string UnaryExpression::toString(int indent) {
  std::string opStr;
  opStr = UnaryOperand2String(op);
  return "(" + opStr + " " + expr->toString(indent + 1) + ")";
}

std::string BinaryExpression::toString(int indent) {
  std::string opStr;
  opStr = BinaryOperand2String(op);
  return "(" + opStr + " " + left->toString(indent + 1) + " " +
         right->toString(indent + 1) + ")";
}

std::string ArrayInitializerExpression::toString(int indent) {
  std::string strOut;
  strOut += "(";
  for (const auto& expr : exprs) {
    strOut += expr->toString(indent + 1) + " ";
  }
  if (!exprs.empty()) {
    strOut.pop_back();
  }
  return strOut + ")";
}

std::string FunctionApplicationExpression::toString(int indent) {
  std::string outStr;
  outStr += "(" + id;
  for (const auto& expr : exprs) {
    outStr += " " + expr->toString(indent + 1);
  }
  return outStr + ")";
}

std::string LengthExpression::toString(int indent) {
  std::string outStr;
  outStr += "(length " + expr->toString(indent + 1) + ")";
  return outStr;
}

std::string IdentifierExpression::toString(int indent) { return id; }

std::string BoolLiteralExpression::toString(int indent) {
  if (value)
    return "true";
  else
    return "false";
}

std::string IntLiteralExpression::toString(int indent) {
  if (value >= 0) {
    return std::to_string(value);
  } else {
    return "(- " +
           std::to_string(0ULL - static_cast<unsigned long long>(value)) + ")";
  }
}

std::string StringLiteralExpression::toString(int indent) {
  return "\"" + value + "\"";
}

std::string CharLiteralExpression::toString(int indent) {
  return "'" + value + "'";
}

// Helper: Corresponding string of base types
std::string BaseType2String(EtaTypeNode::BaseType type) {
  using U = EtaTypeNode::BaseType;
  switch (type) {
    case U::Integer:
      return "int";
    case U::Boolean:
      return "bool";
  }
  return "<unknown-base-type>";
}

std::string BaseTypeNode::toString(int indent) { return BaseType2String(type); }

std::string ArrayTypeNode::toString(int indent) {
  if (dimension)
    return "([] " + subtype->toString(indent + 1) + " " +
           dimension->toString(indent + 1) + ")";
  else
    return "([] " + subtype->toString(indent + 1) + ")";
}

// Helper: Pretty-print Declaration type
std::string DeclarationToString(const int indent, const Declaration& dec) {
  const auto& id = dec.first;
  const auto& type = dec.second;
  return "(" + id + " " + type->toString(indent + 1) + ")";
}

std::string GlobalVariableDeclaration::toString(int indent) {
  std::string declNew =
      DeclarationToString(indent, *declaration)
          .substr(1, DeclarationToString(indent, *declaration).size() - 2);
  if (value)
    return "(:global " + declNew + " " + value->toString(indent + 1) + ")";
  else
    return "(:global " + declNew + ")";
}

std::string ReturnStatement::toString(int indent) {
  std::string outStr = "(return";
  for (const auto& expr : exprs) {
    outStr += " " + expr->toString(indent + 1);
  }
  return outStr + ")";
}

std::string IfStatement::toString(const int indent) {
  std::string indentStr = "";
  for (int i = 0; i < indent; i++) {
    indentStr += "  ";
  }
  std::string outStr = "(if " + condition->toString(indent + 1) + "\n";
  outStr += indentStr + "  " + then_branch->toString(indent + 1) + "\n";

  if (else_branch)
    outStr += indentStr + "  " + else_branch->toString(indent + 1) + "\n";
  else
    outStr += "\n";
  outStr += indentStr + ")";
  return outStr;
}

std::string WhileStatement::toString(int indent) {
  std::string indentStr = "";
  for (int i = 0; i < indent; i++) {
    indentStr += "  ";
  }
  std::string outStr = "(while " + condition->toString(indent + 1) + "\n";
  outStr += indentStr + "  " + body->toString(indent + 1) + "\n";
  outStr += indentStr + ")";
  return outStr;
}

std::string BlockStatement::toString(int indent) {
  std::string indentStr;
  for (int i = 0; i < indent; i++) {
    indentStr += "  ";
  }
  std::string outStr = "(";
  if (!statements.empty()) {
    outStr += "\n";
    for (const auto& statement : statements) {
      outStr += indentStr + "  " + statement->toString(indent + 1) + "\n";
    }
    outStr += indentStr;
  }
  outStr += ")";
  return outStr;
}

std::string ProcedureCall::toString(int indent) {
  std::string outStr;
  outStr += "(" + id;
  for (const auto& expr : arguments) {
    outStr += " " + expr->toString(indent + 1);
  }
  return outStr + ")";
}

std::string DeclarationStatement::toString(int indent) {
  std::string outStr = "";
  if (declarations.size() > 1) outStr += "(";
  for (const auto& [id, type] : declarations) {
    if (type) {
      outStr += "(" + id + " " + type->toString(indent + 1) + ")";
    }
  }
  if (declarations.size() > 1) outStr += ")";
  return outStr;
}

std::string LvalueIdentifierNode::toString(int indent) { return id; }

std::string LvalueArrayNode::toString(int indent) {
  return arrayIndexExpression->toString(indent);
}

std::string AssignmentStatement::toString(const int indent) {
  std::string outStr = "(= ";
  if (lvalues.size() > 1) {
    outStr += "(";
  }
  for (const auto& lvalue : lvalues) {
    outStr += lvalue->toString(indent + 1) + " ";
  }
  outStr.pop_back();
  if (lvalues.size() > 1) {
    outStr += ")";
  }
  outStr += " ";
  if (rvalues.size() > 1) {
    outStr += "(";
  }
  for (const auto& rvalue : rvalues) {
    outStr += rvalue->toString(indent + 1) + " ";
  }
  outStr.pop_back();
  if (rvalues.size() > 1) {
    outStr += ")";
  }
  return outStr + ")";
}

std::string WildcardLvalueNode::toString(int indent) { return "_"; }

std::string DeclarationLvalueNode::toString(int indent) {
  return "(" + declaration->first + " " +
         declaration->second->toString(indent + 1) + ")";
}

// since method_interface is used in method, this function does not have the
// outer parentheses
std::string MethodInterface::toString(const int indent) {
  std::string indentStr;
  for (int i = 0; i < indent; i++) {
    indentStr += "  ";
  }
  std::string outStr = id + "\n" + indentStr + "(";
  if (!declarations.empty()) {
    for (const auto& declaration : declarations) {
      outStr += DeclarationToString(indent + 2, declaration) + " ";
    }
    outStr.pop_back();
  }
  outStr += ")\n" + indentStr + "(";
  if (!return_types.empty()) {
    for (const auto& return_type : return_types) {
      outStr += return_type->toString(indent + 2) + " ";
    }
    outStr.pop_back();
  }
  outStr += ")";
  return outStr;
}

std::string Method::toString(const int indent) {
  std::string indentStr = "";
  for (int i = 0; i < indent; i++) {
    indentStr += "  ";
  }
  std::string outStr = "(\n" + indentStr + "  ";
  outStr += header->toString(indent + 1);
  outStr += "\n" + indentStr + "  " + body->toString(indent + 1) + "\n";
  outStr += indentStr + ")";
  return outStr;
}

std::string Program::toString(const int indent) {
  std::string outStr = "( ";
  if (useClauses().empty())
    outStr += "()";
  else {
    outStr += "(";
    for (auto& use_clause : useClauses()) {
      outStr += use_clause.toString(indent + 2) + " ";
    }
    outStr.pop_back();
    outStr += ")";
  }

  outStr += "\n  (\n";
  for (const auto& definition : definitions) {
    if (dynamic_cast<Use *>(definition.get())) continue;
    outStr += "    ";
    outStr += definition->toString(indent + 2);
    outStr += "\n";
  }
  outStr += "  )\n)";
  return outStr;
}

// std::string Interface::toString(const int indent) {
//   // std::string outStr = "((\n";
//   // for (const auto& method_interface : method_interfaces) {
//   //   outStr += "  (";
//   //   outStr += method_interface->toString(indent + 2);
//   //   outStr += ")\n";
//   // }
//   // outStr += "))";
//   // return outStr;
//   return "";
// }

  std::string BreakStatement::toString(const int indent) {
    return "(break)";
  }

  std::string NullLiteralExpression::toString(const int indent) {
    return "null";
  }

  std::string FieldAccessExpression::toString(const int indent) {
  return "(. " + expr->toString(indent + 1) + " " +
        id + ")";
  }

  std::string LvalueRecordNode::toString(const int indent) {
  return "(. " + record->toString(indent + 1) + " " +
      id + ")";
  }

  std::string RecordTypeNode::toString(const int indent) {
    return name;
  }

  std::string RecordDefinition::toString(int indent) {
    std::string indentStr;
    for (int i = 0; i < indent; i++) {
      indentStr += "  ";
    }
    std::string outStr = "(\n" + indentStr + "  ";
    outStr += ":record " + id + "\n" + indentStr + "  (";
    if (!fields.empty()) {
      for (const auto& [key, value] : fields) {
        outStr += "(" + key + " " + value->toString(indent + 2) + ") ";
      }
      outStr.pop_back();
    }
    outStr += ")\n";
    outStr += indentStr +")";
    return outStr;
  }
}  // namespace ssy38::my_compiler::ast