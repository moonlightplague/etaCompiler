#include "ir.hpp"

#include <limits>
#include <type_traits>
#include <unordered_set>
#include <utility>

#include "EtaASTtoIRVisitor.hpp"
#include "cfg.hpp"
#include "format"
#include "utils.hpp"

namespace ssy38::my_compiler::ir {

namespace {  // helpers for evaluate_binary_operator
constexpr int64_t OVERFLOW_RESULT = std::numeric_limits<int64_t>::min();

bool fits_int64(const __int128_t value) {
  return value >=
             static_cast<__int128_t>(std::numeric_limits<int64_t>::min()) &&
         value <= static_cast<__int128_t>(std::numeric_limits<int64_t>::max());
}

int64_t checked_int64(const __int128_t value) {
  return fits_int64(value) ? static_cast<int64_t>(value) : OVERFLOW_RESULT;
}

int64_t int64_from_bits(const uint64_t bits) {
  constexpr uint64_t SIGN_BIT = 1ULL << 63;
  if (bits < SIGN_BIT) return static_cast<int64_t>(bits);
  return std::numeric_limits<int64_t>::min() +
         static_cast<int64_t>(bits - SIGN_BIT);
}
}  // namespace

/**
 * Calculates the value of a binary operation with bound check
 *
 * @param left the left operand
 * @param right the right operand
 * @param op the binary operator
 * @return result of the operation, std::numeric_limits<int64_t>::min() if
 * result overflows
 */
int64_t evaluate_binary_operator(const int64_t left, const int64_t right,
                                 const BinaryOperator op) {
  using B = BinaryOperator;
  switch (op) {
    case B::Add:
      return checked_int64(static_cast<__int128_t>(left) + right);
    case B::Sub:
      return checked_int64(static_cast<__int128_t>(left) - right);
    case B::Mul: {
      const __int128_t product = static_cast<__int128_t>(left) * right;
      return int64_from_bits(static_cast<uint64_t>(product));
    }
    case B::Hmul: {
      const __int128_t product = static_cast<__int128_t>(left) * right;
      return static_cast<int64_t>(product >> 64);
    }
    case B::Div:
      if (right == 0 ||
          (left == std::numeric_limits<int64_t>::min() && right == -1)) {
        return OVERFLOW_RESULT;
      }
      return left / right;
    case B::Mod:
      if (right == 0 ||
          (left == std::numeric_limits<int64_t>::min() && right == -1)) {
        return OVERFLOW_RESULT;
      }
      return left % right;
    case B::And:
      return left & right;
    case B::Or:
      return left | right;
    case B::Xor:
      return left ^ right;
    case B::Lshift:
      if (right < 0 || right >= 64) return OVERFLOW_RESULT;
      return checked_int64(static_cast<__int128_t>(left) *
                           (static_cast<__int128_t>(1) << right));
    case B::Rshift:
      if (right < 0 || right >= 64) return OVERFLOW_RESULT;
      return int64_from_bits(static_cast<uint64_t>(left) >> right);
    case B::Arshift: {
      if (right < 0 || right >= 64) return OVERFLOW_RESULT;
      const __int128_t divisor = static_cast<__int128_t>(1) << right;
      __int128_t quotient = static_cast<__int128_t>(left) / divisor;
      const __int128_t remainder = static_cast<__int128_t>(left) % divisor;
      if (left < 0 && remainder != 0) --quotient;
      return checked_int64(quotient);
    }
    case B::Eq:
      return left == right;
    case B::Neq:
      return left != right;
    case B::Lt:
      return left < right;
    case B::Ult:
      return static_cast<uint64_t>(left) < static_cast<uint64_t>(right);
    case B::Gt:
      return left > right;
    case B::Leq:
      return left <= right;
    case B::Geq:
      return left >= right;
  }
  return OVERFLOW_RESULT;
}

void hash_combine(std::size_t &seed, const std::size_t value) {
  seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
}

std::size_t hash_tag(const std::string &tag) {
  return std::hash<std::string>{}(tag);
}

std::size_t vn::vnExprHash::operator()(const vn::vnExpr &expr) const {
  std::size_t seed = hash_tag("vnExpr");
  hash_combine(seed, std::hash<unsigned int>{}(expr.left));
  hash_combine(seed, std::hash<unsigned int>{}(expr.right));
  hash_combine(seed, std::hash<int>{}(static_cast<int>(expr.op)));
  return seed;
}

bool vn::vnExprEqual::operator()(const vn::vnExpr &lhs,
                                 const vn::vnExpr &rhs) const {
  return lhs.left == rhs.left && lhs.right == rhs.right && lhs.op == rhs.op;
}

std::unordered_set<std::string> IntegerLiteral::temps_of() {
  return std::unordered_set<std::string>();
}

std::unordered_set<std::string> Temporary::temps_of() {
  return std::unordered_set<std::string>{name};
}

std::unordered_set<std::string> BinaryExpression::temps_of() {
  auto left_set = left->temps_of();
  auto right_set = right->temps_of();
  left_set.insert(right_set.begin(), right_set.end());
  return left_set;
}

std::unordered_set<std::string> MemoryAccess::temps_of() {
  return expr->temps_of();
}

std::unordered_set<std::string> FunctionCall::temps_of() {
  throw std::runtime_error(
      "FunctionCall expression not allowed in cfg analysis");
}

std::unordered_set<std::string> Address::temps_of() {
  return std::unordered_set<std::string>();
}

std::unordered_set<std::string> ExpressionSequence::temps_of() {
  throw std::runtime_error("ExpressionSequence not allowed in cfg analysis");
}

std::any IntegerLiteral::rewrite(const std::unordered_set<cfg::copy> &A) {
  return std::static_pointer_cast<Expression>(
      std::make_shared<IntegerLiteral>(*this));
}

std::any Temporary::rewrite(const std::unordered_set<cfg::copy> &A) {
  return std::static_pointer_cast<Expression>(
      std::make_shared<Temporary>(cfg::canon(A, name)));
}

std::any BinaryExpression::rewrite(const std::unordered_set<cfg::copy> &A) {
  return std::static_pointer_cast<Expression>(
      std::make_shared<BinaryExpression>(
          std::any_cast<std::shared_ptr<Expression>>(left->rewrite(A)),
          std::any_cast<std::shared_ptr<Expression>>(right->rewrite(A)), op));
}

std::any MemoryAccess::rewrite(const std::unordered_set<cfg::copy> &A) {
  return std::static_pointer_cast<Expression>(std::make_shared<MemoryAccess>(
      std::any_cast<std::shared_ptr<Expression>>(expr->rewrite(A))));
}

std::any FunctionCall::rewrite(const std::unordered_set<cfg::copy> &A) {
  throw std::runtime_error(
      "FunctionCall expression not allowed in cfg analysis");
}

std::any Address::rewrite(const std::unordered_set<cfg::copy> &A) {
  return std::static_pointer_cast<Expression>(std::make_shared<Address>(*this));
}

std::any ExpressionSequence::rewrite(const std::unordered_set<cfg::copy> &A) {
  throw std::runtime_error(
      "ExpressionSequence expression not allowed in cfg analysis");
}

int64_t IntegerLiteral::CCP_eval(const cfg::const_map &A) {
  return this->value;
}

int64_t Temporary::CCP_eval(const cfg::const_map &A) {
  std::string name = this->name;
  auto it = A.find(name);
  if (it == A.end())
    throw std::runtime_error(std::format("{} undefined in CCP analysis", name));
  return it->second;
}

int64_t BinaryExpression::CCP_eval(const cfg::const_map &A) {
  int64_t left = this->left->CCP_eval(A);
  int64_t right = this->right->CCP_eval(A);
  if (left != std::numeric_limits<int64_t>::min() &&
      right != std::numeric_limits<int64_t>::min())
    return evaluate_binary_operator(left, right, op);
  else
    return std::numeric_limits<int64_t>::min();
}

int64_t MemoryAccess::CCP_eval(const cfg::const_map &A) {
  return std::numeric_limits<int64_t>::min();
}

int64_t FunctionCall::CCP_eval(const cfg::const_map &A) {
  throw std::runtime_error(
      "FunctionCall expression not allowed in CCP analysis");
}

int64_t Address::CCP_eval(const cfg::const_map &A) {
  return std::numeric_limits<int64_t>::min();
}

int64_t ExpressionSequence::CCP_eval(const cfg::const_map &A) {
  throw std::runtime_error(
      "ExpressionSequence expression not allowed in CCP analysis");
}

std::any IntegerLiteral::const_rewrite(const cfg::const_map &A) {
  return std::static_pointer_cast<Expression>(
      std::make_shared<IntegerLiteral>(*this));
}

std::any Temporary::const_rewrite(const cfg::const_map &A) {
  auto constant = CCP_eval(A);
  if (constant != std::numeric_limits<int64_t>::min())
    return std::static_pointer_cast<Expression>(
        std::make_shared<IntegerLiteral>(constant));
  return std::static_pointer_cast<Expression>(
      std::make_shared<Temporary>(*this));
}

std::any BinaryExpression::const_rewrite(const cfg::const_map &A) {
  auto constant = CCP_eval(A);
  if (constant != std::numeric_limits<int64_t>::min())
    return std::static_pointer_cast<Expression>(
        std::make_shared<IntegerLiteral>(constant));
  return std::static_pointer_cast<Expression>(
      std::make_shared<BinaryExpression>(
          std::any_cast<std::shared_ptr<Expression>>(left->const_rewrite(A)),
          std::any_cast<std::shared_ptr<Expression>>(right->const_rewrite(A)),
          op));
}

std::any MemoryAccess::const_rewrite(const cfg::const_map &A) {
  return std::static_pointer_cast<Expression>(std::make_shared<MemoryAccess>(
      std::any_cast<std::shared_ptr<Expression>>(expr->const_rewrite(A))));
}

std::any FunctionCall::const_rewrite(const cfg::const_map &A) {
  throw std::runtime_error(
      "FunctionCall expression not allowed in cfg analysis");
}

std::any Address::const_rewrite(const cfg::const_map &A) {
  return std::static_pointer_cast<Expression>(std::make_shared<Address>(*this));
}

std::any ExpressionSequence::const_rewrite(const cfg::const_map &A) {
  throw std::runtime_error(
      "ExpressionSequence expression not allowed in cfg analysis");
}

std::tuple<unsigned int, std::any> IntegerLiteral::vn_key_rewrite(
    vn::vnTable &A) {
  if (A.constVN.contains(value))
    return {A.constVN[value], std::static_pointer_cast<Expression>(
                                  std::make_shared<IntegerLiteral>(*this))};
  A.constVN[value] = A.current_value;
  A.current_value++;
  return {A.current_value - 1, std::static_pointer_cast<Expression>(
                                   std::make_shared<IntegerLiteral>(*this))};
}

std::tuple<unsigned int, std::any> Temporary::vn_key_rewrite(vn::vnTable &A) {
  if (A.tempVN.contains(name))
    return {A.tempVN[name], std::static_pointer_cast<Expression>(
                                std::make_shared<Temporary>(*this))};
  A.tempVN[name] = A.current_value;
  A.vnTemps[A.current_value] = {name};
  A.current_value++;
  return {A.current_value - 1, std::static_pointer_cast<Expression>(
                                   std::make_shared<Temporary>(*this))};
}

std::tuple<unsigned int, std::any> BinaryExpression::vn_key_rewrite(
    vn::vnTable &A) {
  auto [left_key, left_any] = left->vn_key_rewrite(A);
  auto [right_key, right_any] = right->vn_key_rewrite(A);
  if(op == BinaryOperator::Add || op == BinaryOperator::Mul){
    if(left_key > right_key) std::swap(left_key, right_key);
  }
  auto result_expr =
      std::static_pointer_cast<Expression>(std::make_shared<BinaryExpression>(
          std::any_cast<std::shared_ptr<Expression>>(left_any),
          std::any_cast<std::shared_ptr<Expression>>(right_any), op));
  vn::vnExpr pattern(left_key, right_key, op);
  if (A.vnExprVN.contains(pattern)) {
    auto v = A.vnExprVN[pattern];
    if (A.vnTemps.contains(v) && !A.vnTemps[v].empty()) {
      const auto &representative = *A.vnTemps[v].begin();
      return {v, std::static_pointer_cast<Expression>(
                     std::make_shared<Temporary>(representative))};
    }
    return {v, result_expr};
  }
  A.vnExprVN[pattern] = A.current_value;
  A.current_value++;
  return {A.current_value - 1, result_expr};
}

std::tuple<unsigned int, std::any> MemoryAccess::vn_key_rewrite(
    vn::vnTable &A) {
  auto [key, expr_any] = expr->vn_key_rewrite(A);
  auto result_expr =
      std::static_pointer_cast<Expression>(std::make_shared<MemoryAccess>(
          std::any_cast<std::shared_ptr<Expression>>(expr_any)));
  if (A.memVN.contains(key)) return {A.memVN[key], result_expr};
  A.memVN[key] = A.current_value;
  A.current_value++;
  return {A.current_value - 1, result_expr};
}

std::tuple<unsigned int, std::any> FunctionCall::vn_key_rewrite(
    vn::vnTable &A) {
  throw std::runtime_error(
      "FunctionCall expression not allowed in VN analysis");
}

std::tuple<unsigned int, std::any> Address::vn_key_rewrite(vn::vnTable &A) {
  auto result_expr =
      std::static_pointer_cast<Expression>(std::make_shared<Address>(*this));
  if (A.addressVN.contains(label)) return {A.addressVN[label], result_expr};
  A.addressVN[label] = A.current_value;
  A.current_value++;
  return {A.current_value - 1, result_expr};
}

std::tuple<unsigned int, std::any> ExpressionSequence::vn_key_rewrite(
    vn::vnTable &A) {
  throw std::runtime_error("ExpressionSequence not allowed in VN analysis");
}

/**
 * A string that contains a specified number of spaces.
 *
 * @param indent the number of spaces
 * @return a string with the specified number of spaces
 */
std::string indentString(const int indent) {
  std::string output(indent, ' ');
  return output;
}

/**
 * Transforms a binary operator to a string.
 *
 * @param op the binary operator
 * @return the string representation of the binary operand
 */
std::string BinaryOperator2String(const BinaryOperator op) {
  using B = BinaryOperator;
  switch (op) {
    case B::Mul:
      return "MUL";
    case B::Hmul:
      return "HMUL";
    case B::Div:
      return "DIV";
    case B::Mod:
      return "MOD";
    case B::Add:
      return "ADD";
    case B::Sub:
      return "SUB";
    case B::And:
      return "AND";
    case B::Or:
      return "OR";
    case B::Xor:
      return "XOR";
    case B::Lshift:
      return "LSHIFT";
    case B::Rshift:
      return "RSHIFT";
    case B::Arshift:
      return "ARSHIFT";
    case B::Eq:
      return "EQ";
    case B::Neq:
      return "NEQ";
    case B::Lt:
      return "LT";
    case B::Ult:
      return "ULT";
    case B::Gt:
      return "GT";
    case B::Leq:
      return "LEQ";
    case B::Geq:
      return "GEQ";
  }
  return "<unknown-binary-op>";
}

/**
 * Encodes a name according to the Eta ABI specification.
 * Replaces each underscore in name with two underscores.
 * Replaces each ' with $1.
 *
 * @param name the name
 * @return the name with two underscores for each underscore
 */
std::string abi_encoded_name(const std::string &name) {
  std::string name_encoding;
  name_encoding.reserve(name.size() * 2);
  for (const char c : name) {
    if (c == '_') {
      name_encoding += "__";
    } else if (c == '\'') {
      name_encoding += "$1";
    } else {
      name_encoding += c;
    }
  }
  return name_encoding;
}

std::string abi_encoding_of_function(std::string name, types::GammaType gamma) {
  auto entry = gamma.find(name);
  if (auto entryMethodType = std::get_if<types::MethodType>(entry)) {
    std::string name_encoding = abi_encoded_name(name);

    std::string return_encoding;
    if (entryMethodType->second.empty()) {
      return_encoding = "p";
    } else if (entryMethodType->second.size() == 1) {
      return_encoding = entryMethodType->second[0]->toAbiString();
    } else {
      return_encoding = std::format("t{}", entryMethodType->second.size());
      for (const auto &return_value_type : entryMethodType->second) {
        return_encoding += return_value_type->toAbiString();
      }
    }

    std::string params_encoding;
    for (const auto &param_type : entryMethodType->first) {
      params_encoding += param_type->toAbiString();
    }

    return std::format("_I{}_{}{}", name_encoding, return_encoding,
                       params_encoding);
  } else {
    throw std::invalid_argument(std::format("{} is not a function", name));
  }
}

std::string abi_encoding_of_global(std::string name) {
  std::string name_encoding = abi_encoded_name(name);

  return std::format("_V{}", name_encoding);
}

/**
 * Creates a fresh label with the supplied name.
 *
 * @param name name of the label.
 * @return the created label.
 */
std::string create_label_name(std::string name) {
  static int id = 0;
  id++;
  return std::format("_L{}_{}", name, id);
}

/**
 * Creates a fresh temporary using the name supplied.
 *
 * @param name name of the temporary.
 * @return the created temporary.
 */
Temporary create_temporary(std::string name) {
  return Temporary(utils::create_temporary_string(std::move(name)));
}

/**
 * Gets the temporary of a local variable.
 *
 * @param name - name of the local variable
 * @return the associated temporary.
 */
Temporary temporary_of(std::string name) {
  return Temporary(std::format("_X{}", abi_encoded_name(name)));
}

std::string DataDeclaration::toString(int indent) {
  std::string dataString = "";
  bool first = true;
  for (const auto &d : data) {
    if (!first) {
      dataString += " ";
    }
    dataString += std::format("{}", d);
    first = false;
  }
  std::string output =
      std::format("{}(DATA {} ({}))", indentString(indent), name, dataString);
  return output;
}

std::string FunctionDeclaration::toString(int indent) {
  std::string output = std::format("{}(FUNC {}\n{})", indentString(indent),
                                   name, body->toString(indent + 2));
  return output;
}

std::string CompilationUnit::toString(int indent) {
  std::string dataString = "";
  for (auto &data : dataDeclarations) {
    dataString += std::format("\n{}", data.toString(indent + 2));
  }
  std::string functionString = "";
  for (auto &function : functionDeclarations) {
    functionString += std::format("\n{}", function.toString(indent + 2));
  }
  std::string output =
      std::format("(COMPUNIT {}{}{})", name, dataString, functionString);
  return output;
}

std::string IntegerLiteral::toString(int indent) {
  std::string output = std::format("(CONST {})", value);
  return output;
}

std::string Temporary::toString(int indent) {
  std::string output = std::format("(TEMP {})", name);
  return output;
}

std::string BinaryExpression::toString(int indent) {
  std::string output = std::format("({} {} {})", BinaryOperator2String(op),
                                   left->toString(0), right->toString(0));
  return output;
}

std::string MemoryAccess::toString(int indent) {
  std::string output = std::format("(MEM {})", expr->toString(indent + 2));
  return output;
}

std::string FunctionCall::toString(int indent) {
  std::string argString = "";
  for (const auto &arg : arguments) {
    if (argString.empty())
      argString = arg->toString(indent + 2);
    else
      argString = std::format("{} {}", argString, arg->toString(indent + 2));
  }
  std::string output =
      std::format("(CALL {} {})", function->toString(indent + 2), argString);
  return output;
}

std::string Address::toString(int indent) {
  std::string output = std::format("(NAME {})", label);
  return output;
}

std::string ExpressionSequence::toString(int indent) {
  std::string output =
      std::format("(ESEQ\n{} {})", statement->toString(indent + 2),
                  expression->toString(indent + 2));
  return output;
}

std::string Assignment::toString(int indent) {
  const std::string destinationString = std::visit(
      utils::overloaded{
          [indent](Temporary &t) { return t.toString(indent + 2); },
          [indent](MemoryAccess &m) { return m.toString(indent + 2); }},
      dest);
  std::string output =
      std::format("{}(MOVE {} {})", indentString(indent), destinationString,
                  expression->toString(indent + 2));
  return output;
}

std::string StatementSequence::toString(int indent) {
  std::string stmtString = "";
  for (auto &stmt : statements) {
    stmtString = std::format("{}\n{}", stmtString, stmt->toString(indent + 2));
  }
  std::string output =
      std::format("{}(SEQ{})", indentString(indent), stmtString);
  return output;
}

std::string Jump::toString(int indent) {
  std::string output = std::format("{}(JUMP {})", indentString(indent),
                                   target->toString(indent + 2));
  return output;
}

std::string Label::toString(int indent) {
  std::string output = std::format("{}(LABEL {})", indentString(indent), name);
  return output;
}

std::string ConditionalJump::toString(int indent) {
  if (!elseLabelName.empty()) {
    throw std::runtime_error(
        "elseLabelName should always be empty after control-flow process");
    // std::string output = std::format("{}(CJUMP {} {} {})",
    // indentString(indent),condition->toString(indent + 2),
    //                                  ifLabelName, elseLabelName);
  }
  std::string output =
      std::format("{}(CJUMP {} {})", indentString(indent),
                  condition->toString(indent + 2), ifLabelName);
  return output;
}

std::string CallStatement::toString(int indent) {
  std::string argString = "";
  for (const auto &arg : arguments) {
    if (argString.empty())
      argString = arg->toString(indent + 2);
    else
      argString = std::format("{} {}", argString, arg->toString(indent + 2));
  }
  std::string output =
      std::format("{}(CALL_STMT {} {} {})", indentString(indent), numReturns,
                  function->toString(indent + 2), argString);
  return output;
}

std::string ReturnStatement::toString(int indent) {
  if (results.empty()) return std::format("{}(RETURN)", indentString(indent));
  std::string exprString = "";
  for (const auto &expr : results) {
    if (exprString.empty())
      exprString = expr->toString(indent + 2);
    else
      exprString = std::format("{} {}", exprString, expr->toString(indent + 2));
  }
  std::string output =
      std::format("{}(RETURN {})", indentString(indent), exprString);
  return output;
}
}  // namespace ssy38::my_compiler::ir
