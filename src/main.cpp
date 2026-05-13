#include <CLI11.hpp>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "antlr4-runtime.h"
#include "generated/etaLex.h"
#include <utils.hpp>
#include "generated/etaParse.h"
#include "EtaParseTreeVisitor.hpp"
#include "asm_driver.hpp"
#include "ir_driver.hpp"
#include "lexer_driver.hpp"
#include "opflags.hpp"
#include "parser_driver.hpp"
#include "typechecker_driver.hpp"
using namespace ssy38;

namespace {

std::string shell_quote(const std::filesystem::path& path) {
  std::string input = path.string();
  std::string output = "'";
  for (const char c : input) {
    if (c == '\'') {
      output += "'\\''";
    } else {
      output += c;
    }
  }
  output += "'";
  return output;
}

std::filesystem::path find_runtime_dir(const char* argv0) {
  const auto cwd_runtime = std::filesystem::current_path() / "runtime";
  if (std::filesystem::exists(cwd_runtime / "libeta.a")) {
    return cwd_runtime;
  }

  std::filesystem::path executable_path(argv0);
  if (executable_path.has_parent_path()) {
    const auto executable_runtime =
        std::filesystem::absolute(executable_path).parent_path() / "runtime";
    if (std::filesystem::exists(executable_runtime / "libeta.a")) {
      return executable_runtime;
    }
  }

  throw std::runtime_error(
      "Eta runtime is not built. Run the CMake build first to produce runtime/libeta.a.");
}

std::filesystem::path default_binary_path(const std::string& output_dir) {
  const std::filesystem::path dir =
      output_dir.empty() ? std::filesystem::path(".")
                         : std::filesystem::path(output_dir);
  return dir / "binary";
}

void link_eta_binary(const std::vector<std::filesystem::path>& asm_paths,
                     const std::filesystem::path& binary_path,
                     const std::filesystem::path& runtime_dir) {
  if (asm_paths.empty()) {
    throw std::runtime_error("No assembly inputs were generated.");
  }

  if (const auto parent = binary_path.parent_path(); !parent.empty()) {
    std::filesystem::create_directories(parent);
  }

  std::ostringstream cmd;
  cmd << "gcc -fno-pie -no-pie -o " << shell_quote(binary_path);
  for (const auto& asm_path : asm_paths) {
    cmd << " " << shell_quote(asm_path);
  }
  cmd << " -L" << shell_quote(runtime_dir)
      << " -L" << shell_quote(runtime_dir / "ncurses-6.3" / "lib")
      << " -leta -lncurses -lpthread";

  const int status = std::system(cmd.str().c_str());
  if (status != 0) {
    throw std::runtime_error("Linking generated assembly failed.");
  }
}

}  // namespace

bool cf = true;
bool reg = true;
bool copy = true;
bool dce = true;
bool cp = true;
bool vn = true;
std::vector<std::string> optcfgPhases;
std::vector<std::string> optirPhases;

int main(int argc, char* argv[]) {

  CLI::App app{"etac"};
  app.allow_non_standard_option_names();
  app.set_help_flag();

  bool help_flag = false;
  bool lex_flag = false;
  bool parse_flag = false;
  bool typecheck_flag = false;
  bool irgen_flag = false;
  bool irrun_flag = false;
  bool asmgen_flag = false;
  bool optimizations_flag = false;
  bool report_optimizations_flag = false;

  std::string diagnosticDir, sourceDir, libDir, asmDir, targetOS, binaryPath;
  const std::vector<std::string> supportedOptimizationFlags = {
      "cf", "reg", "copy", "dce", "cp", "vn"};
  std::vector<std::string> optimizationFlags;
  std::vector<std::string> sources;
  // CLI flag setup
  app.add_flag("--help", help_flag, "Synopsis of options");
  app.add_flag("--report-opts", report_optimizations_flag,
               "List optimizations supported by compiler");
  app.add_flag("--lex", lex_flag, "Generate output from lexical analysis");
  app.add_flag("--parse", parse_flag, "Generate output from parser analysis");
  app.add_flag("--typecheck", typecheck_flag,
               "Generate output from semantic analysis");
  app.add_flag("--irgen", irgen_flag, "Generate intermediate code");
  app.add_flag("--irrun", irrun_flag,
               "Generate and interpret intermediate code");
  app.add_flag("--asmgen", asmgen_flag, "Generate assembly output");
  auto* optimizationOption =
      app.add_flag("-O", optimizations_flag,
                   "Select which optimizations to apply")
          ->type_name("<opt>...:{cf,reg,copy,dce,cp,vn}");
  app.add_option("--optcfg", optcfgPhases,
                 "Generate CFG DOT output for optimization phase(s): initial and/or final")
      ->check(CLI::IsMember({"initial", "final"}))
      ->expected(1, 2)
      ->type_name("<phase>...");
  app.add_option("--optir", optirPhases,
                 "Generate intermediate code output for optimization phase(s): initial and/or final")
      ->check(CLI::IsMember({"initial", "final"}))
      ->expected(1, 2)
      ->type_name("<phase>...");
  app.add_option("-sourcepath", sourceDir, "Path to load the target files")
      ->type_name("<path>")
      ->default_val("");
  app.add_option("-libpath", libDir, "Path to load the library interface files")
      ->type_name("<path>")
      ->default_val("");
  app.add_option("-D", diagnosticDir,
                 "Path to place generated diagnostic files")
      ->type_name("<path>")
      ->default_val("");
  app.add_option("-d", asmDir, "Path to place generated assembly output files")
      ->type_name("<path>")
      ->default_val("");
  app.add_option("-o", binaryPath, "Path to place generated executable binary")
      ->type_name("<path>")
      ->default_val("");
  app.add_option("-target", targetOS,
                 "Operating system for which to generate code; OS may be one of linux, windows, and macos")
      ->type_name("<OS>")
      ->default_val("linux");
  app.add_option("sources", sources, "Source files");

  //look for input optimizations options in supported optimiations
  auto is_supported_optimization = [&](const std::string& opt) {
    return std::find(supportedOptimizationFlags.begin(),
                     supportedOptimizationFlags.end(),
                     opt) != supportedOptimizationFlags.end();
  };

  // Prevent source filenames from being consumed as -O operands while still
  // accepting the spec's combined form, e.g. -Oreg.
  std::vector<std::string> filteredArgs;
  filteredArgs.reserve(argc);
  filteredArgs.emplace_back(argv[0]);
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg.rfind("-O", 0) == 0 && arg.size() > 2) {
      const std::string opt = arg.substr(2);
      if (is_supported_optimization(opt)) {
        filteredArgs.emplace_back("-O");
        optimizationFlags.emplace_back(opt);
        continue;
      }
    }

    filteredArgs.push_back(arg);
    if (arg != "-O") {
      continue;
    }
    while (i + 1 < argc && is_supported_optimization(argv[i + 1])) {
      optimizationFlags.emplace_back(argv[++i]);
    }
  }
  std::vector<char*> filteredArgv;
  filteredArgv.reserve(filteredArgs.size());
  for (auto& arg : filteredArgs) {
    filteredArgv.push_back(arg.data());
  }
  argc = static_cast<int>(filteredArgv.size());
  argv = filteredArgv.data();

  CLI11_PARSE(app, argc, argv);

  // assign optimization flags
  if (optimizationOption->count() == 0) {
    cf = true;
    reg = true;
    copy = true;
    dce = true;
    cp = true;
    vn = true;
  } else {
    cf = false;
    reg = false;
    copy = false;
    dce = false;
    cp = false;
    vn = false;
    for (const auto& opt : optimizationFlags) {
      if (opt == "cf") {
        cf = true;
      } else if (opt == "reg") {
        reg = true;
      } else if (opt == "copy") {
        copy = true;
      } else if (opt == "dce") {
        dce = true;
      } else if (opt == "cp") {
        cp = true;
      } else if (opt == "vn") {
        vn = true;
      }
    }
  }

  if (report_optimizations_flag) {
    std::cout << "cf\n"
              << "reg\n"
              << "copy\n"
              << "dce\n"
              << "cp\n"
              << "vn\n";
    return 0;
  }
  // Prints a synopsis of options when help flag raised or input file empty.
  if (help_flag || sources.empty()) {
    std::cout << "Usage: etac [options] <source files>\n";
    for (const auto* opt : app.get_options()) {
      if (opt->get_name() == "sources") {
        continue;
      }
      std::cout << "  " << opt->get_name() << " " << opt->get_type_name()
                << "\t\t" << opt->get_description() << "\n";
    }
    return 0;
  }

  if (targetOS != "linux") {
    std::cout
        << "Our compiler currently only supports the Linux operating system."
        << std::endl;
    return 0;
  }

  if (lex_flag) {
    ssy38::my_compiler::lex_sources(sources, diagnosticDir, sourceDir);
    return 0;
  }

  if (parse_flag) {
    ssy38::my_compiler::parse_sources(sources, diagnosticDir, sourceDir);
    return 0;
  }

  if (typecheck_flag) {
    ssy38::my_compiler::typecheck_sources(sources, diagnosticDir, sourceDir,
                                          libDir);
    return 0;
  }

  if (irgen_flag) {
    ssy38::my_compiler::generate_ir_for_sources(
        sources, diagnosticDir, sourceDir, libDir,
        false);
    return 0;
  }

  if (irrun_flag) {
    ssy38::my_compiler::generate_ir_for_sources(
        sources, diagnosticDir, sourceDir, libDir,
        true);
    return 0;
  }

  if (asmgen_flag) {
    ssy38::my_compiler::generate_asm_for_sources(
        sources, sourceDir, libDir, asmDir);
    return 0;
  }

  try {
    const auto asm_paths = ssy38::my_compiler::generate_asm_paths_for_sources(
        sources, sourceDir, libDir, asmDir);
    const auto output_binary =
        binaryPath.empty() ? default_binary_path(asmDir)
                           : std::filesystem::path(binaryPath);
    link_eta_binary(asm_paths, output_binary, find_runtime_dir(argv[0]));
    for (const auto& asm_path : asm_paths) {
      std::error_code ec;
      std::filesystem::remove(asm_path, ec);
    }
  } catch (const std::exception& e) {
    std::cout << e.what() << "\n";
    return 1;
  }

  return 0;
}
