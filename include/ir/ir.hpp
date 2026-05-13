#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include "EtaIRVisitor.hpp"
#include "copy.hpp"
#include "types.hpp"

namespace ssy38::my_compiler::ir {
/**
 * Encodes a function into the ABI name.
 * Example: main(args: int[][]) --> _Imain_paai
 *
 * @param name name of the function.
 * @param gamma typing context containing the function.
 * @return the ABI name of a function.
 */
std::string abi_encoding_of_function(std::string name, types::GammaType gamma);

/**
 * Encodes a global into the ABI name.
 * Example: g: int -> _Gg
 *
 * @param name name of the global variable.
 * @return the ABI name of a global variable.
 */
std::string abi_encoding_of_global(std::string name);

/**
 * Creates a fresh/unique label name with the supplied name.
 * Example: l1 -> l1_10
 *
 * @param name name of the label.
 * @return the created label.
 */
std::string create_label_name(std::string name);

/**
 * Creates a fresh temporary using the name supplied.
 *
 * @param name name of the temporary.
 * @return the created temporary.
 */
Temporary create_temporary(std::string name);

/**
 * Gets the temporary of a local variable.
 *
 * @param name - name of the local variable
 * @return the associated temporary.
 */
Temporary temporary_of(std::string name);

struct IRNode {
  virtual ~IRNode() = default;

  /**
   * Pretty-print an IR Node in an S-expression format.
   *
   * @param indent - The indentation (number of spaces) before each
   * S-expression.
   *
   * @return The pretty-printed string of the IR Node in S-expression format.
   */
  virtual std::string toString(int indent) = 0;

  /**
   * Accepts a visitor and performs double dispatch.
   *
   * @param visitor - the visitor to accept
   * @return the created value
   */
  virtual std::any accept(visitor::EtaIRVisitor &visitor) = 0;
};

struct Statement : public IRNode {};

struct DataDeclaration : public IRNode {
  std::string name;
  std::vector<int64_t> data;

  DataDeclaration(std::string name, std::vector<int64_t> data)
      : name(std::move(name)), data(std::move(data)) {}

  std::string toString(int indent) override;
  std::any accept(visitor::EtaIRVisitor &visitor) override {
    return visitor.visit(*this);
  }
};

struct FunctionDeclaration : public IRNode {
  std::string name;
  std::shared_ptr<Statement> body;
  std::size_t arg_count;
  std::size_t return_val_count;

  FunctionDeclaration(std::string name, std::shared_ptr<Statement> body,
                      std::size_t arg_count, std::size_t return_val_count)
      : name(std::move(name)),
        body(std::move(body)),
        arg_count(std::move(arg_count)),
        return_val_count(std::move(return_val_count)) {}

  std::string toString(int indent) override;
  std::any accept(visitor::EtaIRVisitor &visitor) override {
    return visitor.visit(*this);
  }
};

struct CompilationUnit : public IRNode {
  std::string name;
  std::vector<DataDeclaration> dataDeclarations;
  std::vector<FunctionDeclaration> functionDeclarations;

  CompilationUnit(std::string name,
                  std::vector<DataDeclaration> dataDeclarations,
                  std::vector<FunctionDeclaration> functionDeclarations)
      : name(std::move(name)),
        dataDeclarations(std::move(dataDeclarations)),
        functionDeclarations(std::move(functionDeclarations)) {}

  std::string toString(int indent = 0) override;
  std::any accept(visitor::EtaIRVisitor &visitor) override {
    return visitor.visit(*this);
  }
};

enum class BinaryOperator {
  Add,
  Sub,
  Mul,
  Hmul,
  Div,
  Mod,
  And,
  Or,
  Xor,
  Lshift,
  Rshift,
  Arshift,
  Eq,
  Neq,
  Lt,
  Ult,
  Gt,
  Leq,
  Geq
};

/**
 * Transforms a binary operator to a string.
 *
 * @param op the binary operator
 * @return the string representation of the binary operand
 */
std::string BinaryOperator2String(const BinaryOperator op);

namespace vn {
struct vnTable;
}

// Expressions
struct Expression : public IRNode {
  /**
   * Extract all the involving temporaries in an expression (used for dead code
   * elimination)
   *
   * @return An unordered_set including all the temporaries
   */
  virtual std::unordered_set<std::string> temps_of() = 0;

  /**
   * Rewrite the expression to its propagated form (used for copy propagation)
   *
   * @return A rewritten expression
   */
  virtual std::any rewrite(const std::unordered_set<cfg::copy> &A) = 0;

  /**
   * Evaluate the constant value for an expression (used for constant
   * propagation)
   *
   * @return An integer representing the value of the current expression
   */
  virtual int64_t CCP_eval(const cfg::const_map &A) = 0;

  /**
   * Rewrite the expression to its constant-propagated form (used for constant
   * propagation)
   *
   * @return A rewritten expression
   */
  virtual std::any const_rewrite(const cfg::const_map &A) = 0;

  /**
   * calculate the vn value for an expression and update the vntable (used for
   * value numbering)
   *
   * @return A tuple of vn value of an expression and the rewritten expression
   */
  virtual std::tuple<unsigned int, std::any> vn_key_rewrite(vn::vnTable &A) = 0;
};

struct IntegerLiteral : public Expression {
  int64_t value;

  explicit IntegerLiteral(const int64_t value) : value(value) {}

  std::unordered_set<std::string> temps_of() override;
  std::any rewrite(const std::unordered_set<cfg::copy> &A) override;
  int64_t CCP_eval(const cfg::const_map &A) override;
  std::any const_rewrite(const cfg::const_map &A) override;
  std::tuple<unsigned int, std::any> vn_key_rewrite(vn::vnTable &A) override;
  std::string toString(int indent) override;
  std::any accept(visitor::EtaIRVisitor &visitor) override {
    return visitor.visit(*this);
  }
};

struct Temporary : public Expression {
  std::string name;

  explicit Temporary(std::string name) : name(std::move(name)) {}

  std::unordered_set<std::string> temps_of() override;
  std::any rewrite(const std::unordered_set<cfg::copy> &A) override;
  int64_t CCP_eval(const cfg::const_map &A) override;
  std::any const_rewrite(const cfg::const_map &A) override;
  std::tuple<unsigned int, std::any> vn_key_rewrite(vn::vnTable &A) override;
  std::string toString(int indent) override;
  std::any accept(visitor::EtaIRVisitor &visitor) override {
    return visitor.visit(*this);
  }
};

struct BinaryExpression : public Expression {
  std::shared_ptr<Expression> left;
  std::shared_ptr<Expression> right;
  BinaryOperator op;

  BinaryExpression(std::shared_ptr<Expression> left,
                   std::shared_ptr<Expression> right, const BinaryOperator op)
      : left(std::move(left)), right(std::move(right)), op(op) {}

  std::unordered_set<std::string> temps_of() override;
  std::any rewrite(const std::unordered_set<cfg::copy> &A) override;
  int64_t CCP_eval(const cfg::const_map &A) override;
  std::any const_rewrite(const cfg::const_map &A) override;
  std::tuple<unsigned int, std::any> vn_key_rewrite(vn::vnTable &A) override;
  std::string toString(int indent) override;
  std::any accept(visitor::EtaIRVisitor &visitor) override {
    return visitor.visit(*this);
  }
};

struct MemoryAccess : public Expression {
  std::shared_ptr<Expression> expr;

  explicit MemoryAccess(std::shared_ptr<Expression> expr)
      : expr(std::move(expr)) {}

  std::unordered_set<std::string> temps_of() override;
  std::any rewrite(const std::unordered_set<cfg::copy> &A) override;
  int64_t CCP_eval(const cfg::const_map &A) override;
  std::any const_rewrite(const cfg::const_map &A) override;
  std::tuple<unsigned int, std::any> vn_key_rewrite(vn::vnTable &A) override;
  std::string toString(int indent) override;
  std::any accept(visitor::EtaIRVisitor &visitor) override {
    return visitor.visit(*this);
  }
};

struct FunctionCall : public Expression {
  std::shared_ptr<Expression> function;
  std::vector<std::shared_ptr<Expression>> arguments;

  FunctionCall(std::shared_ptr<Expression> function,
               std::vector<std::shared_ptr<Expression>> arguments)
      : function(std::move(function)), arguments(std::move(arguments)) {}

  std::unordered_set<std::string> temps_of() override;
  std::any rewrite(const std::unordered_set<cfg::copy> &A) override;
  int64_t CCP_eval(const cfg::const_map &A) override;
  std::any const_rewrite(const cfg::const_map &A) override;
  std::tuple<unsigned int, std::any> vn_key_rewrite(vn::vnTable &A) override;
  std::string toString(int indent) override;
  std::any accept(visitor::EtaIRVisitor &visitor) override {
    return visitor.visit(*this);
  }
};

struct Address : public Expression {
  std::string label;

  explicit Address(std::string label) : label(std::move(label)) {}

  std::unordered_set<std::string> temps_of() override;
  std::any rewrite(const std::unordered_set<cfg::copy> &A) override;
  int64_t CCP_eval(const cfg::const_map &A) override;
  std::any const_rewrite(const cfg::const_map &A) override;
  std::tuple<unsigned int, std::any> vn_key_rewrite(vn::vnTable &A) override;
  std::string toString(int indent) override;
  std::any accept(visitor::EtaIRVisitor &visitor) override {
    return visitor.visit(*this);
  }
};

struct ExpressionSequence : public Expression {
  std::shared_ptr<Statement> statement;
  std::shared_ptr<Expression> expression;

  ExpressionSequence(std::shared_ptr<Statement> statement,
                     std::shared_ptr<Expression> expression)
      : statement(std::move(statement)), expression(std::move(expression)) {}

  std::unordered_set<std::string> temps_of() override;
  std::any rewrite(const std::unordered_set<cfg::copy> &A) override;
  int64_t CCP_eval(const cfg::const_map &A) override;
  std::any const_rewrite(const cfg::const_map &A) override;
  std::tuple<unsigned int, std::any> vn_key_rewrite(vn::vnTable &A) override;
  std::string toString(int indent) override;
  std::any accept(visitor::EtaIRVisitor &visitor) override {
    return visitor.visit(*this);
  }
};

namespace vn {
struct vnExpr {
  unsigned int left;
  unsigned int right;
  BinaryOperator op;
  vnExpr(unsigned int left, unsigned int right, BinaryOperator op)
      : left(left), right(right), op(op) {}
};
struct vnExprHash {
  std::size_t operator()(const vnExpr &expr) const;
};

struct vnExprEqual {
  bool operator()(const vnExpr &lhs, const vnExpr &rhs) const;
};
struct vnTable {
  std::unordered_map<std::string, unsigned int> tempVN;
  std::unordered_map<unsigned int, std::unordered_set<std::string>> vnTemps;
  std::unordered_map<vnExpr, unsigned int, vnExprHash, vnExprEqual> vnExprVN;
  std::unordered_map<int64_t, unsigned int> constVN;
  std::unordered_map<int64_t, unsigned int> memVN;
  std::unordered_map<std::string, unsigned int> addressVN;
  unsigned int current_value;
  vnTable()
      : tempVN(std::unordered_map<std::string, unsigned int>{}),
        vnTemps(std::unordered_map<unsigned int,
                                   std::unordered_set<std::string>>{}),
        vnExprVN(std::unordered_map<vnExpr, unsigned int, vnExprHash,
                                    vnExprEqual>{}),
        constVN(std::unordered_map<int64_t, unsigned int>{}),
        memVN(std::unordered_map<int64_t, unsigned int>{}),
        addressVN(std::unordered_map<std::string, unsigned int>{}),
        current_value(0) {}
};
}  // namespace vn

using Destination = std::variant<Temporary, MemoryAccess>;

// Statements

struct Assignment : public Statement {
  Destination dest;
  std::shared_ptr<Expression> expression;

  Assignment(Destination dest, std::shared_ptr<Expression> expression)
      : dest(std::move(dest)), expression(std::move(expression)) {}

  std::string toString(int indent) override;
  std::any accept(visitor::EtaIRVisitor &visitor) override {
    return visitor.visit(*this);
  }
};

struct StatementSequence : public Statement {
  std::vector<std::shared_ptr<Statement>> statements;

  explicit StatementSequence(std::vector<std::shared_ptr<Statement>> statements)
      : statements(std::move(statements)) {}

  std::string toString(int indent) override;
  std::any accept(visitor::EtaIRVisitor &visitor) override {
    return visitor.visit(*this);
  }
};

struct Jump : public Statement {
  std::shared_ptr<Expression> target;

  explicit Jump(std::shared_ptr<Expression> target)
      : target(std::move(target)) {}

  std::string toString(int indent) override;
  std::any accept(visitor::EtaIRVisitor &visitor) override {
    return visitor.visit(*this);
  }
};

struct Label : public Statement {
  std::string name;

  explicit Label(std::string name) : name(std::move(name)) {}

  std::string toString(int indent) override;
  std::any accept(visitor::EtaIRVisitor &visitor) override {
    return visitor.visit(*this);
  }
};

struct ConditionalJump : public Statement {
  std::shared_ptr<Expression> condition;
  std::string ifLabelName;
  std::string elseLabelName;

  ConditionalJump(std::shared_ptr<Expression> condition,
                  std::string ifLabelName)
      : ConditionalJump(std::move(condition), std::move(ifLabelName), "") {}

  ConditionalJump(std::shared_ptr<Expression> condition,
                  std::string ifLabelName, std::string elseLabelName)
      :  // condition(std::make_shared<BinaryExpression>(std::move(condition),
         // std::make_shared<IntegerLiteral>(0), BinaryOperator::Neq)), // TODO:
         // If they update the interpreter, we can get rid of this extra !=
         // expression
        condition(std::move(condition)),
        ifLabelName(std::move(ifLabelName)),
        elseLabelName(std::move(elseLabelName)) {}

  std::string toString(int indent) override;
  std::any accept(visitor::EtaIRVisitor &visitor) override {
    return visitor.visit(*this);
  }
};

struct CallStatement : public Statement {
  std::shared_ptr<Expression> function;
  std::vector<std::shared_ptr<Expression>> arguments;
  int numReturns;

  CallStatement(std::shared_ptr<Expression> function,
                std::vector<std::shared_ptr<Expression>> arguments,
                const int numReturns)
      : function(std::move(function)),
        arguments(std::move(arguments)),
        numReturns(numReturns) {}

  std::string toString(int indent) override;
  std::any accept(visitor::EtaIRVisitor &visitor) override {
    return visitor.visit(*this);
  }
};

struct ReturnStatement : public Statement {
  std::vector<std::shared_ptr<Expression>> results;

  explicit ReturnStatement(std::vector<std::shared_ptr<Expression>> results)
      : results(std::move(results)) {}

  std::string toString(int indent) override;
  std::any accept(visitor::EtaIRVisitor &visitor) override {
    return visitor.visit(*this);
  }
};
}  // namespace ssy38::my_compiler::ir
