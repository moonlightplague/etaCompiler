#include "utils.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

namespace ssy38::utils {

std::string create_temporary_string(std::string name) {
  static int id = 0;
  id++;
  return std::format("_T{}_{}", name, id);
}

std::string codepoint_to_utf8(uint32_t cp) {
  std::string out;
  if (cp <= 0x7F) {
    out += static_cast<char>(cp);
  } else if (cp <= 0x7FF) {
    out += static_cast<char>(0xC0 | ((cp >> 6) & 0x1F));
    out += static_cast<char>(0x80 | (cp & 0x3F));
  } else if (cp <= 0xFFFF) {
    out += static_cast<char>(0xE0 | ((cp >> 12) & 0x0F));
    out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
    out += static_cast<char>(0x80 | (cp & 0x3F));
  } else if (cp <= 0x10FFFF) {
    out += static_cast<char>(0xF0 | ((cp >> 18) & 0x07));
    out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
    out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
    out += static_cast<char>(0x80 | (cp & 0x3F));
  } else {
    // Replacement character (U+FFFD) in UTF-8.
    return "\xEF\xBF\xBD";
  }
  return out;
}

/**
 * Convert a raw Eta string to its escaped form and the list of codepoints in
 * its escaped form
 * @param input the raw Eta string
 * @return a pair, the first element being its escaped form (an std::string,
 * internally represented as a byte stream) and a vector of the list of UTF-8
 * codepoints in its escaped form.
 */
std::pair<std::string, std::vector<uint64_t>> escape_string(
    const std::string& input) {
  std::string result;
  std::vector<uint64_t> codepoints;
  result.reserve(input.length());
  codepoints.reserve(input.length());

  for (size_t i = 0; i < input.length(); ++i) {
    // If the current character is not a backslash, always consume.
    // If you are at the last character, always consume.
    if (input[i] != '\\' || i == input.length() - 1) {
      codepoints.push_back(input[i]);
      result += input[i];
      continue;
    }

    // Otherwise, you are currently in an escape character.
    char escaped = input[++i];

    switch (escaped) {
      case '\\':
        result += '\\';
        codepoints.push_back('\\');
        break;
      case '\'':
        result += '\'';
        codepoints.push_back('\'');
        break;
      case 'n':
        result += "\\n";
        codepoints.push_back('\n');
        break;
      case 'x': {  // Example: \x{64} -> d
        bool is_valid_escape = false;

        // Enters the hexadecimal digits.
        if (i + 1 < input.length() && input[i + 1] == '{') {
          size_t start_hex = i + 2;  // First hex digit.
          size_t end_brace = input.find('}', start_hex);

          if (end_brace != std::string::npos && end_brace - start_hex <= 6) {
            std::string hex_str =
                input.substr(start_hex, end_brace - start_hex);
            bool should_throw = false;
            try {
              size_t parsed_count = 0;  // Number of chars parsed.
              uint32_t codepoint = std::stoul(hex_str, &parsed_count, 16);
              // if codepoint is too large, we should error
              if (parsed_count == hex_str.length()) {
                if (codepoint > 0x10FFFF) {
                  should_throw = true;
                }
                std::string converted = codepoint_to_utf8(codepoint);
                if (converted == "\n") {
                  converted = R"(\n)";
                }
                result += converted;
                codepoints.push_back(codepoint);
                i = end_brace;
                is_valid_escape = true;
              }
            } catch (...) {
              // Catch error thrown from std::stoul
              // Flag remains false, skip, don't throw
            }
            if (should_throw) {
              throw std::invalid_argument(
                  "Invalid Unicode character of codepoint " + hex_str);
            }
          }
        }

        // If invalid, just treat "\x" as raw characters.
        if (!is_valid_escape) {
          result += "\\x";
          codepoints.push_back('\\');
          codepoints.push_back('x');
        }
        break;
      }
      default:
        // Example: \b -> \b
        result += '\\';
        result += escaped;
        codepoints.push_back('\\');
        codepoints.push_back(escaped);
        break;
    }
  }
  return {result, codepoints};
}

std::string read_file(const std::string& source_filename,
                      const std::string& directory) {
  fs::path input_path(source_filename);

  if (input_path.extension().string() != ".eta" &&
      input_path.extension().string() != ".eti" &&
      input_path.extension().string() != ".rh" &&
      input_path.extension().string() != ".ri") {
    throw std::invalid_argument("Input file " + source_filename +
                                " does not have the correct extension.");
  }

  fs::path dir_path = directory.empty() ? fs::path(".") : fs::path(directory);

  fs::path rel_parent = input_path.parent_path();
  if (input_path.is_absolute()) {
    rel_parent = input_path.parent_path().relative_path();
  }

  fs::path pa = dir_path / rel_parent / input_path.filename();

  if (fs::path parent = pa.parent_path(); !parent.empty()) {
    fs::create_directories(parent);
  }

  std::ifstream in(pa, std::ios::binary);
  if (!in) {
    throw std::runtime_error("Unable to open input file " + source_filename);
  }
  return std::string(std::istreambuf_iterator<char>(in),
                     std::istreambuf_iterator<char>());
}

std::vector<std::string> find_interface_files(const std::string& directory) {
  fs::path dir_path = directory.empty() ? fs::path(".") : fs::path(directory);

  if (!fs::exists(dir_path) || !fs::is_directory(dir_path)) {
    throw std::invalid_argument("Directory " +
                                (directory.empty() ? "." : directory) +
                                " does not exist or is not a directory.");
  }

  std::vector<std::string> interface_files;

  for (const auto& entry : fs::directory_iterator(dir_path)) {
    if (!entry.is_regular_file()) continue;

    const std::string ext = entry.path().extension().string();
    if (ext == ".eti" || ext == ".ri") {
      interface_files.push_back(entry.path().filename().string());
    }
  }

  return interface_files;
}

std::ofstream open_output(const std::string& source_filename,
                          const std::string& directory,
                          const CompilerMode& compiler_mode) {
  fs::path input_path(source_filename);

  fs::path dir_path = directory.empty() ? fs::path(".") : fs::path(directory);

  fs::path rel_parent = input_path.parent_path();
  if (input_path.is_absolute()) {
    rel_parent = input_path.parent_path().relative_path();
  }

  fs::path output_path = dir_path / rel_parent / input_path.filename();
  switch (compiler_mode) {
    case CompilerMode::Lex:
      output_path.replace_extension(".lexed");
      break;
    case CompilerMode::Parse:
      output_path.replace_extension(".parsed");
      break;
    case CompilerMode::TypeCheck:
      output_path.replace_extension(".typed");
      break;
    case CompilerMode::IR:
      output_path.replace_extension(".ir");
      break;
    case CompilerMode::ASM:
      output_path.replace_extension(".s");
      break;
  }

  if (fs::path parent = output_path.parent_path(); !parent.empty()) {
    fs::create_directories(parent);
  }

  std::ofstream out(output_path, std::ios::binary);
  if (!out) {
    throw std::runtime_error("Unable to open file " + output_path.string() +
                             " for writing");
  }
  return out;
}
}  // namespace ssy38::utils