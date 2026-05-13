#include "ir_driver.hpp"

#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "EtaASTtoIRVisitor.hpp"
#include "EtaConstantFolder.hpp"
#include "EtaIRControlFlowAnalyzer.hpp"
#include "EtaIRtoCFGVisitor.hpp"
#include "EtaMIRtoLIRVisitor.hpp"
#include "ast.hpp"
#include "opflags.hpp"
#include "typechecker_driver.hpp"
#include "utils.hpp"

namespace fs = std::filesystem;

namespace ssy38::my_compiler {
ir::CompilationUnit build_ir(std::ofstream& fout,
                             const std::string& source_filename,
                             const std::string& source_directory,
                             const std::string& lib_directory,
                             const std::string& optcfg_dir) {
  std::unique_ptr<ast::Program> programAST;
  // Do Eta Frontend processing (lex, parse, typecheck)
  try {
    programAST =
        typecheck_one_file(source_filename, source_directory, lib_directory);
  } catch (const utils::EtaFrontendError& e) {
    std::cout << e.toConsoleString(source_filename) << std::endl;
    fout << e.toFileString() << std::endl;
  } catch (const utils::EtaDriverError& e) {
    if (e.printToConsole) {
      std::cout << e.what() << std::endl;
    }
    if (e.printToFile) {
      fout << e.what() << std::endl;
    }
  }
  if (!programAST) {
    throw std::runtime_error("Error when building AST");
  }

  // AST -> IR
  visitor::EtaASTtoIRVisitor irVisitor;
  auto programIR =
      std::any_cast<ir::CompilationUnit>(programAST->accept(irVisitor));

  ir::CompilationUnit midIR = [&]() {
    if (!cf) {
      return programIR;
    } else {
      // Fold constants
      visitor::EtaConstantFolder foldVisitor;
      return std::any_cast<ir::CompilationUnit>(programIR.accept(foldVisitor));
    }
  }();

  // MIR -> LIR
  visitor::EtaMIRtoLIRVisitor lowerVisitor;
  auto loweredIR =
      std::any_cast<ir::CompilationUnit>(midIR.accept(lowerVisitor));

  // Reorder IR
  visitor::EtaIRControlFlowAnalyzer reorderVisitor;
  auto reorderedIR =
      std::any_cast<ir::CompilationUnit>(loweredIR.accept(reorderVisitor));

  if (std::find(optirPhases.begin(), optirPhases.end(), "initial") !=
      optirPhases.end()) {
    fs::path input_path(source_filename);
    std::string file_stem = input_path.stem().string();
    auto initial_filename = std::format("{}_initial.eta", file_stem);
    std::ofstream optir_initial = utils::open_output(
        initial_filename, optcfg_dir, utils::CompilerMode::IR);
    optir_initial << reorderedIR.toString(0) << std::endl;
  }

  // cfg optimizations
  visitor::EtaIRtoCFGVisitor DCEVisitor(optcfgPhases, optcfg_dir,
                                        source_filename);
  auto cfgOptimizedIR =
      std::any_cast<ir::CompilationUnit>(reorderedIR.accept(DCEVisitor));

  if (std::find(optirPhases.begin(), optirPhases.end(), "final") !=
      optirPhases.end()) {
    fs::path input_path(source_filename);
    std::string file_stem = input_path.stem().string();
    auto final_filename = std::format("{}_final.eta", file_stem);
    std::ofstream optir_final =
        utils::open_output(final_filename, optcfg_dir, utils::CompilerMode::IR);
    optir_final << cfgOptimizedIR.toString(0) << std::endl;
  }
  return cfgOptimizedIR;
}

/**
 * Generates intermediate code for the specified source file and writes a *.ir
 * file. Optionally interprets the code and prints any interpreted output to
 * stdout.
 *
 * @param source_filename - the source file to type check.
 * @param out_directory - where to place the generated diagnostic file.
 * @param source_directory - directory that the source file is in.
 * @param lib_directory - directory that the interface files are in.
 * @param interpret - interprets the generated intermediate code.
 */
void generate_ir_one_file(const std::string& source_filename,
                          const std::string& out_directory,
                          const std::string& source_directory,
                          const std::string& lib_directory, bool interpret) {
  // Output filestream setup.
  std::ofstream fout = utils::open_output(source_filename, out_directory,
                                          utils::CompilerMode::IR);

  // IR structure fetch.
  std::optional<ir::CompilationUnit> resultIR;
  try {
    resultIR = build_ir(fout, source_filename, source_directory, lib_directory);
  } catch (const std::exception& e) {
    return;
  }

  fout << resultIR->toString(0) << std::endl;

  if (interpret) {
    // get generated ir file path
    fs::path input_path(source_filename);
    fs::path dir_path =
        out_directory.empty() ? fs::path(".") : fs::path(out_directory);
    fs::path rel_parent = input_path.parent_path();
    if (input_path.is_absolute()) {
      rel_parent = input_path.parent_path().relative_path();
    }
    fs::path ir_path = dir_path / rel_parent / input_path.filename();
    ir_path.replace_extension(".ir");

    // run interpreter on the ir
    std::string cmd = "java -jar tools/interpreter.jar " + ir_path.string();
    [[maybe_unused]] const int interpreter_status = std::system(cmd.c_str());
  }
}

void generate_ir_for_sources(const std::vector<std::string>& sources,
                             const std::string& out_directory,
                             const std::string& source_directory,
                             const std::string& lib_directory, bool interpret) {
  for (const auto& source_filename : sources) {
    try {
      generate_ir_one_file(source_filename, out_directory, source_directory,
                           lib_directory, interpret);
    } catch (const std::exception& e) {
      std::cout << e.what() << "\n";
    }
  }
}
}  // namespace ssy38::my_compiler
