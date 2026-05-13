#include "lexer_driver.hpp"
#include <filesystem>

#include <algorithm>
#include <cctype>
#include <exception>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <utils.hpp>
#include <vector>

#include "antlr4-runtime.h"
#include "generated/etaLex.h"

namespace ssy38::my_compiler {
namespace {

/**
 * Runs lexical analysis for the specified source file and writes a *.lexed
 * file.
 *
 * @param source_filename - the source file to lex
 * @param out_directory - where to place the generated diagnostic file.
 * @param source_directory - directory that the source file is in.
 */
void lex_one_file(const std::string& source_filename,
                  const std::string& out_directory,
                  const std::string& source_directory) {
  // Sends file content into lexer.
  antlr4::ANTLRInputStream input(
      utils::read_file(source_filename, source_directory));
  std::filesystem::path source_filepath(source_filename);
  etaLex lexer(&input);
  if (source_filepath.extension().string() == ".rh" || source_filepath.extension().string() == ".ri") {
    lexer.isRho = true;
  }
  // Replace default error listeners.
  lexer.removeErrorListeners();
  utils::EtaFrontendErrorListener lex_err(utils::CompilerMode::Lex);
  lexer.addErrorListener(&lex_err);

  const antlr4::dfa::Vocabulary& vocabulary = lexer.getVocabulary();

  // Output filestream setup.
  std::ofstream fout =
      open_output(source_filename, out_directory, utils::CompilerMode::Lex);

  try {
    while (true) {
      std::unique_ptr<antlr4::Token> token(lexer.nextToken());

      // Stops token output at EOF.
      if (token->getType() == antlr4::Token::EOF) {
        break;
      }

      // Outputs lines & columns of target tokens.
      std::string token_type(vocabulary.getSymbolicName(token->getType()));
      if (token_type == "WS" || token_type == "LINE_COMMENT") {
        continue;
      }
      fout << token->getLine() << ":" << (token->getCharPositionInLine() + 1)
           << " ";

      // Converts token type to lowercases.
      std::transform(
          token_type.begin(), token_type.end(), token_type.begin(),
          [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

      // Required token type output.
      if (token_type == "id" || token_type == "integer" ||
          token_type == "character" || token_type == "string") {
        fout << token_type << " ";
      }

      // Removes the quotes and escapes when certain token type detected.
      if (token_type == "character" || token_type == "string") {
        std::string text(token->getText());
        text.erase(0, 1);
        text.pop_back();
        try {
          fout << utils::escape_string(text).first << "\n";
        } catch (const std::invalid_argument& e) {
          throw utils::EtaFrontendError(
              utils::CompilerMode::Lex, token->getLine(),
              token->getCharPositionInLine() + 1, e.what());
        }
      } else {
        fout << token->getText() << "\n";
      }
    }
  } catch (const utils::EtaFrontendError& error) {
    std::cout << error.toConsoleString(source_filename) << std::endl;
    fout << error.toFileString() << "\n";
  }
}

}  // namespace
}  // namespace ssy38::my_compiler

namespace ssy38::my_compiler {

void lex_sources(const std::vector<std::string>& sources,
                 const std::string& out_directory,
                 const std::string& source_directory) {
  for (const auto& source_filename : sources) {
    try {
      lex_one_file(source_filename, out_directory, source_directory);
    } catch (const std::exception& e) {
      std::cerr << e.what() << "\n";
    }
  }
}

}  // namespace ssy38::my_compiler