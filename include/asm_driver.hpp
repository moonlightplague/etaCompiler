#pragma once
#include <string>
#include <vector>

#include "asm.hpp"
namespace ssy38::my_compiler {
/**
 * Generates assembly code for each source in `sources` and writes *.s
 * outputs. If a source fails (I/O error, etc.), it reports to stdout and
 * continues to the next source.
 *
 * @param sources - the source files to compile.
 * @param source_directory - directory that the source files are in.
 * @param lib_directory - directory that the interface files are in.
 * @param asm_directory - where to place generated assembly files.
 */
void generate_asm_for_sources(const std::vector<std::string>& sources,
                         const std::string& source_directory,
                         const std::string& lib_directory,
                         const std::string& asm_directory);
}  // namespace ssy38::my_compiler
