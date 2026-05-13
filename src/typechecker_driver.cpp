#include "typechecker_driver.hpp"

#include <filesystem>
#include <format>
#include <iostream>
#include <string>

#include "EtaParseTreeVisitor.hpp"
#include "EtaTypeChecker.hpp"
#include "etaLex.h"
#include "etaParse.h"
#include "immer/map_transient.hpp"
#include "utils.hpp"

namespace fs = std::filesystem;

namespace ssy38::my_compiler {
namespace {
/**
 * Combines the type contexts (gammas) of the program gamma and interface
 * gammas.
 *
 * @param program_gamma - the program gamma.
 * @param interfaces - a vector of interface gammas paired with the interface
 * info.
 * @return the combined gamma (type context).
 * @throws utils::EtaFrontendError If an interface's method's type signature
 * mismatches/conflicts with another method/variable.
 * @throws std::invalid_argument If an interface's gamma contains something
 * other than a method type.
 */
types::GammaType merge_gammas(
    const types::GammaType& program_gamma,
    const std::vector<std::pair<types::UseInterface, types::GammaType>>&
        interfaces) {
  auto combined_gamma = program_gamma.transient();

  for (const auto& [interface_info, interface_gamma] : interfaces) {
    for (const auto& [key, entry] : interface_gamma) {
      if (auto* existing = combined_gamma.find(key)) {
        // interfaces only have functions, no variable declarations
        if (auto* entryMethodType = std::get_if<types::MethodType>(&entry)) {
          if (auto* existingMethodType =
                  std::get_if<types::MethodType>(existing)) {
            if (!methodTypesEqual(*entryMethodType, *existingMethodType)) {
              throw utils::EtaFrontendError(
                  utils::CompilerMode::TypeCheck, interface_info.line,
                  interface_info.column,
                  std::format(
                      "{} has a function {} with a mismatched signature",
                      interface_info.name, key));
            }
          } else {
            throw utils::EtaFrontendError(
                utils::CompilerMode::TypeCheck, interface_info.line,
                interface_info.column,
                std::format("{} has a function {} that conflicts with an "
                            "existing identifier",
                            interface_info.name, key));
          }
        } else {
          throw std::invalid_argument(
              "Interfaces can only have method interfaces: interface entries' "
              "types can only be method types");
        }
      } else {
        combined_gamma.set(key, entry);
      }
    }
  }

  return combined_gamma.persistent();
}

void check_conformance(const types::GammaType &programGamma, const types::GammaType &interfaceGamma) {
  for (const auto& [name, entry] : interfaceGamma) {
    if (auto interfaceMethod = std::get_if<types::MethodType>(&entry)) {
      if (!std::get_if<types::RecordDefnType>(interfaceGamma.find(types::getRecordDefnAccessorStr(name)))) {
        auto programMethod = std::get_if<types::MethodType>(programGamma.find(name));
        if (!programMethod || !types::methodTypesEqual(*interfaceMethod, *programMethod)) {
          throw utils::EtaFrontendError(
                 utils::CompilerMode::TypeCheck, 0, 0,
                 std::format(
                     "function with name {} is not implemented properly", name));
        }
      }
    }
    if (auto interfaceRecord = std::get_if<types::RecordDefnType>(&entry)) {
      auto programRecord = std::get_if<types::RecordDefnType>(programGamma.find(name));
      if (!programRecord) {
        throw utils::EtaFrontendError(
                 utils::CompilerMode::TypeCheck, 0, 0,
                 std::format(
                     "record with name {} is not implemented ", name));
      }
      auto interfaceIt = interfaceRecord->begin();
      auto programIt = programRecord->begin();
      for (int i = 0; i < interfaceRecord->size(); i++, ++interfaceIt, ++programIt) {
        if (interfaceIt->first != programIt->first ||
          !interfaceIt->second->leq(*programIt->second) || !programIt->second->leq(*interfaceIt->second)) {
          throw utils::EtaFrontendError(
                 utils::CompilerMode::TypeCheck, 0, 0,
                 std::format(
                     "record with name {} is not implemented properly", name));
        }
      }
    }
  }
}

/**
 * Lexes and parses an interface.
 *
 * @param interface_filename - name of the interface
 * @param lib_directory - directory that the lib files are in.
 * @return A pair of the interface info with the interface AST (gamma)
 */
std::unique_ptr<ast::Program> parse_interface(
    const std::string &interface_filename, const std::string& lib_directory) {

  fs::path source_filepath(interface_filename);
  // Sends interface file content into lexer.
  antlr4::ANTLRInputStream input(
      utils::read_file(interface_filename, lib_directory));
  etaLex lexer(&input);

  if (source_filepath.extension().string() == ".rh" || source_filepath.extension().string() == ".ri") {
    lexer.isRho = true;
  }

  // Replace default error listeners.
  utils::EtaFrontendErrorListener lex_err(utils::CompilerMode::Lex),
      parse_err(utils::CompilerMode::Parse);

  lexer.removeErrorListeners();
  lexer.addErrorListener(&lex_err);

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

  std::any result;
  if (source_filepath.extension().string() == ".eti") {
    result = visitor.visit(parser.interface());
  }
  else if (source_filepath.extension().string() == ".ri") {
    result = visitor.visit(parser.rhoInterface());
  }
  else {
    throw std::invalid_argument("unsupported extension for interfaces");
  }

  return std::unique_ptr<ast::Program>(std::any_cast<ast::Program*>(result));
}

}  // namespace

std::unique_ptr<ast::Program> typecheck_one_file(
    const std::string& source_filename, const std::string& source_directory,
    const std::string& lib_directory) {
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
  if (source_filepath.extension().string() != ".eta" && source_filepath.extension().string() != ".eti" &&
    source_filepath.extension().string() == ".rh" && source_filepath.extension().string() == ".ri") {
    throw std::invalid_argument("Input file " + source_filename +
                                " does not have required extension.");
  }

  // type check and store interface gammas
  types::InterfaceMap interfaces;
  std::vector<std::unique_ptr<ast::Program>> interfaces_vec;
  for (const auto& interface : utils::find_interface_files(lib_directory)) {
    try {
      interfaces_vec.push_back(std::move(parse_interface(interface, lib_directory)));
      interfaces.insert({fs::path(interface).stem(), interfaces_vec.back().get()});
    } catch (const utils::EtaFrontendError& e) {
      const std::string interface_filename = interface;
      std::cout << e.toConsoleString(interface_filename) << std::endl;

      throw utils::EtaDriverError(
          false, true,
          std::format(
              "semantic error for interface {} -- see standard output for more info",
              interface));
    } catch (const std::runtime_error& e) {
      std::cout << e.what() << std::endl;

      throw utils::EtaDriverError(
          false, false,
          std::format("interface file {} cannot be opened",
                      interface));
    } catch (const std::invalid_argument& e) {
      std::cout << e.what() << std::endl;

      throw utils::EtaDriverError(
          false, true,
          std::format("interface {} is invalid",
                      interface));
    }
  }

  std::any result;
  if (source_filepath.extension().string() == ".eta" || source_filepath.extension().string() == ".rh") {
    result = visitor.visit(parser.program());
  }
  else if (source_filepath.extension().string() == ".eti") {
    result = visitor.visit(parser.interface());
  }
  else {
    result = visitor.visit(parser.rhoInterface());
  }
  auto programAST =
      std::unique_ptr<ast::Program>(std::any_cast<ast::Program*>(result));

  programAST->name = source_filepath.stem().string();
  //first pass
  types::FirstPassTypeChecker firstPassChecker(interfaces);
  programAST->accept(firstPassChecker);

  //second pass
  types::SecondPassTypeChecker secondPassChecker(interfaces, true);
  programAST->secondJudgement.input = programAST->firstJudgement.output;
  programAST->accept(secondPassChecker);

  if (source_filepath.extension().string() == ".eta" || source_filepath.extension().string() == ".rh") {
    types::ThirdPassTypeChecker thirdPassChecker(programAST->secondJudgement.output);
    programAST->accept(thirdPassChecker);

    if (auto interface = interfaces.find(fs::path(source_filename).stem()); interface != interfaces.end()) {
      auto* const interfaceAST = interface->second;
      types::FirstPassTypeChecker firstPassCheckerInterface(interfaces);
      interfaceAST->accept(firstPassChecker);
      types::SecondPassTypeChecker secondPassCheckerInterface(interfaces);
      interfaceAST->secondJudgement.input = interfaceAST->firstJudgement.output;
      interfaceAST->accept(secondPassChecker);
      types::ThirdPassTypeChecker thirdPassCheckerInterface(interfaceAST->secondJudgement.output);
      interfaceAST->accept(thirdPassCheckerInterface);
      check_conformance(thirdPassChecker.output, thirdPassCheckerInterface.output);
    }
  }
  //third pass
  return programAST;
}

void typecheck_sources(const std::vector<std::string>& sources,
                       const std::string& out_directory,
                       const std::string& source_directory,
                       const std::string& lib_directory) {
  for (const auto& source_filename : sources) {
    try {
      // Output filestream setup.
      std::ofstream fout = utils::open_output(source_filename, out_directory,
                                              utils::CompilerMode::TypeCheck);
      try {
        typecheck_one_file(source_filename, source_directory, lib_directory);
        fout << "Valid Eta Program" << std::endl;
      } catch (const utils::EtaFrontendError& e) {
        std::cout << e.toConsoleString(source_filename) << std::endl;
        fout << e.toFileString() << "\n";
      } catch (const utils::EtaDriverError& e) {
        if (e.printToConsole) {
          std::cout << e.what() << std::endl;
        }
        if (e.printToFile) {
          fout << e.what() << std::endl;
        }
      }
    } catch (const std::exception& e) {
      std::cout << e.what() << "\n";
    }
  }
}
}  // namespace ssy38::my_compiler