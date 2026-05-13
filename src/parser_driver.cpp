#include "parser_driver.hpp"

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utils.hpp>
#include <vector>

#include "EtaParseTreeVisitor.hpp"
#include "antlr4-runtime.h"
#include "ast.hpp"
#include "generated/etaLex.h"
#include "generated/etaParse.h"

namespace fs = std::filesystem;

namespace ssy38::my_compiler {
namespace {

/**
 * Runs syntactic analysis for the specified source file and writes a *.parsed
 * file.
 *
 * @param source_filename - the source file to parse
 * @param out_directory - where to place the generated diagnostic file.
 * @param source_directory - directory that the source file is in.
 */
void parse_one_file(const std::string& source_filename,
                    const std::string& out_directory,
                    const std::string& source_directory) {

  std::filesystem::path source_filepath(source_filename);

  // Sends file content into lexer.
  antlr4::ANTLRInputStream input(
      utils::read_file(source_filename, source_directory));
  etaLex lexer(&input);

  if (source_filepath.extension().string() == ".rh" || source_filepath.extension().string() == ".ri") {
    lexer.isRho = true;
  }

  // Replace default error listeners.
  utils::EtaFrontendErrorListener lex_err(utils::CompilerMode::Lex),
      parse_err(utils::CompilerMode::Parse);

  lexer.removeErrorListeners();
  lexer.addErrorListener(&lex_err);

  // Output filestream setup.
  std::ofstream fout = utils::open_output(source_filename, out_directory,
                                          utils::CompilerMode::Parse);
  try {
    // Generate stream of tokens
    antlr4::CommonTokenStream tokens(&lexer);
    tokens.fill();
    etaParse parser(&tokens);
    if (source_filepath.extension().string() == ".rh" || source_filepath.extension().string() == ".ri") {
      parser.isRho = true;
    }
    parser.removeErrorListeners();
    parser.addErrorListener(&parse_err);
    auto visitor = EtaParseTreeVisitor(&tokens);

    if (source_filepath.extension().string() == ".eta" || source_filepath.extension().string() == ".rh") {
      etaParse::ProgramContext* tree = parser.program();
      std::any result = visitor.visit(tree);
      auto programAST =
          std::unique_ptr<ast::Program>(std::any_cast<ast::Program*>(result));
      fout << programAST->toString(0) << std::endl;
    } else if (source_filepath.extension().string() == ".eti") {
      etaParse::InterfaceContext* tree = parser.interface();

      std::any result = visitor.visit(tree);
      auto interfaceAST = std::unique_ptr<ast::Program>(
          std::any_cast<ast::Program*>(result));
      fout << interfaceAST->toString(0) << std::endl;
    } else if (source_filepath.extension().string() == ".ri") {
      auto tree = parser.rhoInterface();
      std::any result = visitor.visit(tree);
      auto rhoInterfaceAST = std::unique_ptr<ast::Program>(
        std::any_cast<ast::Program*>(result));
      fout << rhoInterfaceAST->toString(0) << std::endl;
    }
    else {
      throw std::invalid_argument("Input file " + source_filename +
                                 " does not have the required extensions.");
    }
  } catch (const utils::EtaFrontendError& error) {
    // Outputs error message to both stdout and the output file when syntax
    // error detected.
    std::cout << error.toConsoleString(source_filename) << std::endl;
    fout << error.toFileString() << std::endl;
  }
}

}  // namespace

void parse_sources(const std::vector<std::string>& sources,
                   const std::string& out_directory,
                   const std::string& source_directory) {
  for (const auto& source_filename : sources) {
    try {
      parse_one_file(source_filename, out_directory, source_directory);
    } catch (const std::exception& e) {
      std::cout << e.what() << "\n";
    }
  }
}

}  // namespace ssy38::my_compiler