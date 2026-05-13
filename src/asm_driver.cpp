#include "asm_driver.hpp"

#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#include "EtaIRtoASMVisitor.hpp"
#include "EtaTrivialRegisterAllocationVisitor.hpp"
#include "asm.hpp"
#include "ir.hpp"
#include "ir_driver.hpp"
#include "utils.hpp"
#include "AsmCfg.hpp"
#include "opflags.hpp"

namespace ssy38::my_compiler {

/**
 * Generates assembly code for the specified source file and writes a *.s file.
 *
 * @param source_filename - the source file to compile.
 * @param asm_directory - where to place the generated assembly file.
 * @param source_directory - directory that the source file is in.
 * @param lib_directory - directory that the interface files are in.
 */
std::filesystem::path generate_asm_one_file(const std::string& source_filename,
                                            const std::string& source_directory,
                                            const std::string& lib_directory,
                                            const std::string& asm_directory) {
  const auto asm_path =
      utils::output_path_for(source_filename, asm_directory,
                             utils::CompilerMode::ASM);

  // Output filestream setup.
  std::ofstream fout_asm = utils::open_output(source_filename, asm_directory, utils::CompilerMode::ASM);

  // Build IR
  auto ir = build_ir(fout_asm, source_filename, source_directory, lib_directory);

  // IR -> Abstract ASM
  visitor::EtaIRtoASMVisitor asmVisitor;
  auto abstractASM = std::any_cast<codegen::CommandSequence>(ir.accept(asmVisitor));

  if (!reg) {
    visitor::EtaTrivialRegisterAllocationVisitor registerAllocVisitor;
    auto trivialASM =
        std::any_cast<codegen::CommandSequence>(abstractASM.accept(registerAllocVisitor));

    fout_asm << trivialASM.toString() << std::endl;
  } else {
    asm_cfg::EtaGraphColoringRegisterAllocationVisitor registerAllocVisitor;
    auto regallocASM = std::any_cast<codegen::CommandSequence>(abstractASM.accept(registerAllocVisitor));

    fout_asm << regallocASM.toString() << std::endl;
  }

  return asm_path;
}

void generate_asm_for_sources(const std::vector<std::string>& sources,
                         const std::string& source_directory,
                         const std::string& lib_directory,
                         const std::string& asm_directory) {
  for (const auto& source_filename : sources) {
    try {
      generate_asm_one_file(source_filename, source_directory,
                            lib_directory, asm_directory);
    } catch (const std::exception& e) {
      std::cout << e.what() << "\n";
    }
  }
}

std::vector<std::filesystem::path> generate_asm_paths_for_sources(
    const std::vector<std::string>& sources,
    const std::string& source_directory,
    const std::string& lib_directory,
    const std::string& asm_directory) {
  std::vector<std::filesystem::path> asm_paths;
  asm_paths.reserve(sources.size());

  for (const auto& source_filename : sources) {
    asm_paths.push_back(generate_asm_one_file(source_filename, source_directory,
                                             lib_directory, asm_directory));
  }

  return asm_paths;
}
}  // namespace ssy38::my_compiler
