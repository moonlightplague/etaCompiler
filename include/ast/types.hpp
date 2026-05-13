#pragma once

#include <immer/map.hpp>
#include <immer/set.hpp>
#include <memory>
#include <string>
#include <variant>
#include <vector>
#include "utils.hpp"

namespace ssy38::my_compiler::types {
/**
 * The key for a return type to be used in a method's type context (gamma) map
 */
inline const std::string RETURN_TYPE_KEY = "_rho";

inline const std::string BREAK_TYPE_KEY = "_break";

// Ordinary Types

/**
 * An abstract class for ordinary types.
 */
struct OrdinaryType {
  virtual ~OrdinaryType() = default;

  virtual std::string toString() = 0;

  /**
   * Encodes an ordinary type into an ABI string. This function is only
   * implemented for int, bool, and (potentially multidimensional) arrays of
   * ints/bools.
   *
   * @return the ebi encoding of the type
   */
  virtual std::string toAbiString() {
    throw std::runtime_error(
        "Type does not have ABI string encoding");  // default error message
  };

  bool operator==(const OrdinaryType &other) const {
    throw std::runtime_error("equality between types is deprecated");
  }

  bool operator!=(const OrdinaryType &other) const {
    throw std::runtime_error("inequality between types is deprecated");
  }

  /**
   * implements the less-than-or-equal-to partial order on types
   * @param other
   * @return if (this, other) is in the partial order of less-than-or-equal-to.
   */
  virtual bool leq(OrdinaryType &other) const = 0;
};

/**
 * The type for a wild card, which any type can assign to.
 */
struct AnyType : public OrdinaryType {
  ~AnyType() override = default;

  std::string toString() override { return "?"; }

  bool leq(ssy38::my_compiler::types::OrdinaryType &other) const override {
    return dynamic_cast<AnyType *>(&other) != nullptr;
  }
};

/**
 * The type for an element of an empty array.
 * {} = NothingType[]
 */
struct NothingType : public OrdinaryType {
  ~NothingType() override = default;

  std::string toString() override { return "⊥"; }

  bool leq(ssy38::my_compiler::types::OrdinaryType &other) const override {
    return true;
  }
};

/**
 * Type for an int.
 */
struct IntType : public OrdinaryType {
  ~IntType() override {};

  std::string toString() override { return "int"; }

  std::string toAbiString() override { return "i"; }

  bool leq(ssy38::my_compiler::types::OrdinaryType &other) const override {
    return (dynamic_cast<IntType *>(&other) != nullptr) ||
           (dynamic_cast<AnyType *>(&other));
  }
};

/**
 * Type for a bool.
 */
struct BoolType : public OrdinaryType {
  ~BoolType() override {};

  std::string toString() override { return "bool"; }

  std::string toAbiString() override { return "b"; }

  bool leq(ssy38::my_compiler::types::OrdinaryType &other) const override {
    return (dynamic_cast<BoolType *>(&other) != nullptr) ||
           (dynamic_cast<AnyType *>(&other) != nullptr);
  }
};

/**
 * Type for an array.
 */
struct ArrayType : public OrdinaryType {
  std::shared_ptr<OrdinaryType> subType;

  ArrayType(std::shared_ptr<OrdinaryType> subType) : subType(subType) {}

  ~ArrayType() override {}

  std::string toString() override { return subType->toString() + "[]"; }

  std::string toAbiString() override { return "a" + subType->toAbiString(); }

  bool leq(ssy38::my_compiler::types::OrdinaryType &other) const override {
    if (dynamic_cast<AnyType *>(&other) != nullptr) {
      return true;
    } else if (auto *typePtr = dynamic_cast<ArrayType *>(&other)) {
      return subType->leq(*(typePtr->subType));
    } else {
      return false;
    }
  }
};

/**
 * Type for a record (as the type of expression)
 */
struct RecordType : public OrdinaryType {
  std::string name;
  RecordType(std::string name) : name(name) {}
  ~RecordType() override {}
  std::string toString() override { return name; }
  std::string toAbiString() override {
    std::string abiStr = "r" + std::to_string(name.length());
    for (const auto c : name) {
      if (c == '_') {
        abiStr += "__";
      }
      else {
        abiStr += c;
      }
    }
    return abiStr;
  }

  bool leq(OrdinaryType &other) const override {
    if (const auto ptr = dynamic_cast<RecordType *>(&other)) {
      return name == ptr->name;
    }
    return dynamic_cast<AnyType *>(&other) != nullptr;
  }
};

/**
 * Type for null values
 */
struct NullType : public OrdinaryType {
  NullType() = default;
  ~NullType() override = default;
  std::string toString() override { return "null"; }
  std::string toAbiString() override { return "null"; }
  bool leq(OrdinaryType &other) const override {
    return dynamic_cast<NullType *>(&other) != nullptr ||
            dynamic_cast<ArrayType *>(&other) != nullptr ||
              dynamic_cast<RecordType *>(&other) != nullptr ||
                dynamic_cast<AnyType *>(&other) != nullptr;
  }
};

/**
 * The "greater" (more general, less specific) type.
 *
 * @param t1 - a shared_ptr to the first type
 * @param t2 - a shared_ptr to the second type
 * @return a shared_ptr to the greater of the two types. If none of the types
 * are lesser than the other, outputs null.
 */
inline std::shared_ptr<OrdinaryType> greater(std::shared_ptr<OrdinaryType> t1,
                                             std::shared_ptr<OrdinaryType> t2) {
  if (t1->leq(*t2)) {
    return t2;
  } else if (t2->leq(*t1)) {
    return t1;
  } else {
    return nullptr;
  }
}

/**
 * The tuple type: a vector of shared_ptr's to ordinary types.
 */
using TupleType = std::vector<std::shared_ptr<OrdinaryType>>;

/**
 * The statement type (R)
 */
enum class StatementType { Unit, Void };

enum class VariableCategory { Local, Global };

// VariableType = normal variable (var t) with category info (for bookkeeping)
// TupleType = return type (ret T)
// RecordDefnType = defintion of a record type
// PlaceHolder = indication that a name is currently occupied. Used in uninitialized record types and the "breakable" type
// std::pair<TupleType, TupleType> = function (fn T -> T')
using VariableType = std::pair<VariableCategory, std::shared_ptr<OrdinaryType>>;
using ReturnType = TupleType;
using MethodType = std::pair<TupleType, TupleType>;
using RecordDefnType = utils::InsertionOrderedMap<std::string, std::shared_ptr<OrdinaryType>>;
using PlaceHolder = void*;
using TypeContextEntry = std::variant<VariableType, ReturnType, MethodType, RecordDefnType, PlaceHolder>;

// Gamma (the type context) is a map of names to Type Context Entries
using GammaType = immer::map<std::string, TypeContextEntry>;

using visitedSet = immer::set<std::string>;

/**
 * Compares two tuple types
 * @param t1 - the first tuple type
 * @param t2 - the second tuple type
 * @return true if the tuple types are equal, else false
 */
inline bool tupleTypesEqual(const TupleType &t1, const TupleType &t2) {
  if (t1.size() != t2.size()) {
    return false;
  }
  for (size_t i = 0; i < t1.size(); ++i) {
    if (!(t1[i]->leq(*t2[i]) && t2[i]->leq(*t1[i]))) {
      return false;
    }
  }
  return true;
}

/**
 * Compares two method types
 * @param t1 the first method type
 * @param t2 the second method type
 * @return true if the method types are equal, else false
 */
inline bool methodTypesEqual(const MethodType &t1, const MethodType &t2) {
  return tupleTypesEqual(t1.first, t2.first) &&
         tupleTypesEqual(t1.second, t2.second);
}

inline bool recordDefnTypesEqual(const RecordDefnType &t1, const RecordDefnType &t2) {
  if (t1.size() != t2.size()) {
    return false;
  }
  int i;
  auto it1 = t1.cbegin();
  auto it2 = t2.cbegin();
  for (i = 0; i < t1.size(); ++i, ++it1, ++it2) {
    const auto &[fieldName1, type1] = *it1;
    const auto &[fieldName2, type2] = *it2;
    if (fieldName1 != fieldName2 || !type1->leq(*type2) || !type1->leq(*type2)) {
      return false;
    }
  }
  return true;
}

inline std::string getRecordDefnAccessorStr(const std::string& name) {
  return "_R" + name;
}

/**
 * Information about the interface used in the .eta program.
 *
 * name - the name of the interface
 * line - line where the interface is used (read in)
 * column - column where the interface is used (read in)
 */
struct UseInterface {
  std::string name;
  size_t line;
  size_t column;
};

// Typing Judgments, as specified by the Eta Type System Specification
struct ExpressionTypeJudgement {
  GammaType input;
  std::shared_ptr<OrdinaryType> type;
};

struct StatementTypeJudgement {
  GammaType input;

  StatementType type;
  GammaType output;
};

struct FirstPassTypeJudgement {
  GammaType input;
  GammaType output;
  visitedSet visited;
};

struct SecondPassTypeJudgement {
  GammaType input;
  GammaType output;
};

// struct FirstPassMethodTypeJudgement {
//   GammaType input;
//
//   MethodType type;
//   GammaType output;
// };
//
// struct SecondPassMethodTypeJudgement {
//   GammaType input;
//   MethodType type;
// };
//
// struct FirstPassGlobalDeclarationTypeJudgement {
//   GammaType input;
//   GammaType output;
// };
//
// struct SecondPassGlobalDeclarationTypeJudgement {
//   GammaType input;
// };
//
// struct InterfaceTypeJudgement {
//   GammaType output;
// };
//
// struct FirstPassProgramTypeJudgement {
//   GammaType output;
//   std::vector<UseInterface>
//       interfaces;  // used interfaces and where they were used
// };
//
// struct SecondPassProgramTypeJudgement {
//   GammaType input;  // combined, complete input from the used interfaces and
//                     // program itself
// };

// Helper judgement to handle multiple assignments (from section 3 of the Eta
// Type System Specification)
struct LValueNodeTypeJudgement {
  GammaType input;                     // outer judgement (Gamma)
  std::shared_ptr<OrdinaryType> type;  // t
  GammaType current;                   // inner judgement (Gamma')
  GammaType output;                    // output (Gamma'')
};
}  // namespace ssy38::my_compiler::types
