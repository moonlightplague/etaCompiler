#pragma once

#include <cstdint>
#include <filesystem>
#include <format>
#include <string>
#include <vector>

#include "antlr4-runtime.h"

namespace ssy38::utils {

/**
 * Helper struct for std::visit.
 * Source: https://en.cppreference.com/w/cpp/utility/variant/visit2.html
 */
template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
// below is needed for versions lower than C++20
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

/**
 * Defines OS choice.
 */
enum class OS {
  LINUX,
  WINDOWS,
  MACOS
};

/**
 * Creates a fresh temporary id string using the supplied name.
 *
 * @param name name of the temporary.
 * @return the created string of the temporary.
 */
std::string create_temporary_string(std::string name);

/**
 * Represents the mode (flag) that the compiler is called with.
 */
enum class CompilerMode { Lex, Parse, TypeCheck, IR, ASM };

/**
 * A string representation of the compiler mode
 *
 * @param mode - the compiler mode
 * @return the string representation
 */
inline std::string compilerModeToString(const CompilerMode mode) {
  switch (mode) {
    case CompilerMode::Lex:
      return "Lexical";
    case CompilerMode::Parse:
      return "Syntax";
    case CompilerMode::TypeCheck:
      return "Semantic";
    case CompilerMode::IR:
      return "IR";
    case CompilerMode::ASM:
      return "ASM";
    default:
      return "Unknown";
  }
}

/**
 * Converts a Unicode codepoint to a UTF-8 string.
 *
 * @param cp - a codepoint value
 * @return A UTF8 string
 */
std::string codepoint_to_utf8(uint32_t cp);

/**
 * Converts raw input strings into a formatted string where special characters
 * are unescaped, and a list of UTF-8 codepoints of the formatted string
 *
 * Example: "\\x{64}" -> "d"
 * Example: "\\\\"    -> "\\"
 *
 * @param input - a raw input string
 * @return a pair, the first element being the formatted string
 *  and the second element being a list of UTF-8 codepoints of the formatted
 * string
 * @throws std::invalid_argument If the Unicode character is invalid (codepoint
 * > 0x10FFFF)
 */
std::pair<std::string, std::vector<uint64_t>> escape_string(
    const std::string& input);

/**
 * Reads an input file into a string.
 *
 * @param source_filename - The filename of the input file.
 * @param directory - The directory of the input source file.
 * @return the input file's contents as a string.
 * @throws std::invalid_argument if the input file is not .eta or .eti
 * @throws std::runtime_error if the input file cannot be opened
 */
std::string read_file(const std::string& source_filename,
                      const std::string& directory);

/**
 * Opens an output file as an ofstream.
 *
 * @param source_filename - The filename of the input file. The output file has
 * the same name.
 * @param directory - The directory of the input file. The output file will be
 * in the same directory.
 * @param compiler_mode - The flag that the compiler was invoked with. This
 * determines the file extension of the output file.
 * @return an ofstream that can be writen to.
 * @throws std::runtime_error if the output file cannot be opened.
 */
std::ofstream open_output(const std::string& source_filename,
                          const std::string& directory,
                          const CompilerMode& compiler_mode);

/**
 * Computes the output path used by open_output without opening the file.
 */
std::filesystem::path output_path_for(const std::string& source_filename,
                                      const std::string& directory,
                                      const CompilerMode& compiler_mode);

/**
 * Frontend lexing/parsing/typechecking error definition.
 */
struct EtaFrontendError : std::runtime_error {
  CompilerMode mode;
  size_t line;
  size_t column;

  EtaFrontendError(CompilerMode mode, size_t line, size_t column,
                   std::string msg)
      : std::runtime_error(msg),  // Pass the error message to the base class
        mode(mode),
        line(line),
        column(column) {}

  /**
   * The error message in file format.
   *
   * @return The formatted error message.
   */
  std::string toFileString() const {
    return std::format("{}:{} error:{}", line, column, what());
  }

  /**
   * The error message in console format.
   *
   * @param filename - the filename where the error occurred.
   * @return The formatted error message.
   */
  std::string toConsoleString(const std::string& filename) const {
    return std::format("{} error beginning at {}:{}:{}: {}",
                       compilerModeToString(mode), filename, line, column,
                       what());
  }
};

/**
 * Eta driver error, which specifies if it should be printed to console and/or
 * file.
 */
struct EtaDriverError : std::runtime_error {
  bool printToConsole;
  bool printToFile;

  EtaDriverError(bool printToConsole, bool printToFile, std::string msg)
      : std::runtime_error(msg),
        printToConsole(printToConsole),
        printToFile(printToFile) {}
};

/**
 * Custom error listener that stops lexing/parsing when an error is detected and
 * throws the position of the error.
 */
class EtaFrontendErrorListener final : public antlr4::BaseErrorListener {
 public:
  EtaFrontendErrorListener(CompilerMode mode) : mode(mode) {}

  void syntaxError(antlr4::Recognizer* /*recognizer*/,
                   antlr4::Token* /*offendingSymbol*/, size_t line,
                   size_t charPositionInLine, const std::string& msg,
                   std::exception_ptr /*e*/) override {
    throw EtaFrontendError(mode, line, charPositionInLine + 1, msg);
  }

 private:
  CompilerMode mode;
};

    /**
     * Finds all interface files immediately under the given directory.
     *
     * Scans the (non-recursive) contents of @p directory and collects every
     * regular file whose extension is `.eti` or `.ri`. Subdirectories are not
     * traversed.
     *
     * @param directory  Path to the directory to scan. If empty, the current
     *                   working directory (".") is used.
     *
     * @return A vector containing the filenames (name with extension only, no
     *         parent path) of the matching interface files, in the order
     *         produced by the underlying directory iterator (unspecified;
     *         typically filesystem-dependent). The vector is empty if no
     *         matching files are found.
     *
     * @throws std::invalid_argument  If @p directory does not exist or is not
     *                                a directory.
     * @throws std::filesystem::filesystem_error  If a filesystem error occurs
     *                                            while iterating (e.g. permission
     *                                            denied).
     */
    std::vector<std::string> find_interface_files(const std::string& directory);

// ----------------------------------------------------------------------------
// InsertionOrderedMap<Key, Value>
//
// A map that exposes (most of) the std::map interface used for insertion and
// lookup, but whose iterators walk elements in the order they were FIRST
// inserted -- similar to Python's dict or Java's LinkedHashMap.
//
// Internals:
//   * std::vector<std::pair<const Key, Value>>          -- ordered storage,
//                                                          gives O(1) iteration
//                                                          in insertion order.
//   * std::unordered_map<Key, std::size_t, Hash, ...>   -- side index mapping
//                                                          each key to its
//                                                          slot in the vector,
//                                                          giving O(1) average
//                                                          lookup / insert.
//
// deletion is intentionally not implemented: erasing from the middle of the vector would shift indices and
// invalidate every entry in the side map.
// ----------------------------------------------------------------------------
template <typename Key,
          typename Value,
          typename Hash     = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>>
class InsertionOrderedMap {
public:
    using key_type    = Key;
    using mapped_type = Value;
    using value_type  = std::pair<const Key, Value>;
    using size_type   = std::size_t;

    using iterator       = typename std::vector<value_type>::iterator;
    using const_iterator = typename std::vector<value_type>::const_iterator;

    InsertionOrderedMap() = default;

    InsertionOrderedMap(std::initializer_list<value_type> init) {
        items_.reserve(init.size());
        for (const auto& kv : init) insert(kv);
    }

    // --- element access -----------------------------------------------------

    // std::map-style subscript: default-constructs Value if the key is new.
    Value& operator[](const Key& key) {
        if (auto it = index_.find(key); it != index_.end())
            return items_[it->second].second;
        return appendNew(key);
    }

    Value& operator[](Key&& key) {
        if (auto it = index_.find(key); it != index_.end())
            return items_[it->second].second;
        return appendNew(std::move(key));
    }

    Value& at(const Key& key) {
        auto it = index_.find(key);
        if (it == index_.end())
            throw std::out_of_range("InsertionOrderedMap::at: key not found");
        return items_[it->second].second;
    }

    const Value& at(const Key& key) const {
        auto it = index_.find(key);
        if (it == index_.end())
            throw std::out_of_range("InsertionOrderedMap::at: key not found");
        return items_[it->second].second;
    }

    // --- insertion ----------------------------------------------------------

    // Returns {iterator-to-element, was-newly-inserted}.
    // Like std::map, no overwrite happens if the key already exists.
    std::pair<iterator, bool> insert(const value_type& kv) {
        if (auto it = index_.find(kv.first); it != index_.end())
            return {items_.begin() + it->second, false};
        return appendPair(kv);
    }

    std::pair<iterator, bool> insert(value_type&& kv) {
        if (auto it = index_.find(kv.first); it != index_.end())
            return {items_.begin() + it->second, false};
        return appendPair(std::move(kv));
    }

    template <typename... Args>
    std::pair<iterator, bool> emplace(Args&&... args) {
        value_type kv(std::forward<Args>(args)...);
        return insert(std::move(kv));
    }

    // --- lookup -------------------------------------------------------------

    iterator find(const Key& key) {
        auto it = index_.find(key);
        return it == index_.end() ? items_.end() : items_.begin() + it->second;
    }

    const_iterator find(const Key& key) const {
        auto it = index_.find(key);
        return it == index_.end() ? items_.end() : items_.begin() + it->second;
    }

    bool      contains(const Key& key) const { return index_.find(key) != index_.end(); }
    size_type count(const Key& key)    const { return index_.count(key); }

    // --- capacity -----------------------------------------------------------

    size_type size()  const noexcept { return items_.size();  }
    bool      empty() const noexcept { return items_.empty(); }
    void      reserve(size_type n)   { items_.reserve(n); index_.reserve(n); }

    // --- iteration (always insertion order) ---------------------------------

    iterator       begin()        noexcept { return items_.begin();  }
    iterator       end()          noexcept { return items_.end();    }
    const_iterator begin()  const noexcept { return items_.begin();  }
    const_iterator end()    const noexcept { return items_.end();    }
    const_iterator cbegin() const noexcept { return items_.cbegin(); }
    const_iterator cend()   const noexcept { return items_.cend();   }

private:
    // Append a brand-new entry default-constructed at the given key.
    // Strongly exception-safe: if indexing fails, the vector is rolled back.
    template <typename K>
    Value& appendNew(K&& key) {
        const size_type idx = items_.size();
        items_.emplace_back(std::piecewise_construct,
                            std::forward_as_tuple(std::forward<K>(key)),
                            std::forward_as_tuple());
        try {
            index_.emplace(items_.back().first, idx);
        } catch (...) {
            items_.pop_back();
            throw;
        }
        return items_.back().second;
    }

    // Append a pre-built pair; same exception-safety story.
    template <typename P>
    std::pair<iterator, bool> appendPair(P&& kv) {
        const size_type idx = items_.size();
        items_.push_back(std::forward<P>(kv));
        try {
            index_.emplace(items_.back().first, idx);
        } catch (...) {
            items_.pop_back();
            throw;
        }
        return {items_.begin() + idx, true};
    }

    std::vector<value_type>                            items_;
    std::unordered_map<Key, size_type, Hash, KeyEqual> index_;
};
}  // namespace ssy38::utils
