#pragma once
#include <string>
#include <vector>

#include "ir.hpp"

namespace ssy38::my_compiler {
/**
 * Builds IR compilation unit for the specified source file. Writes any errors to
 * fout.
 *
 * @param fout - the file output stream.
 * @param source_filename - the source file to type check.
 * @param source_directory - directory that the source file is in.
 * @param lib_directory - directory that the interface files are in.
 * @param optcfg_dir - where to output the cfg visualization code file
 */
ir::CompilationUnit build_ir(std::ofstream& fout,
                                          const std::string& source_filename,
                                          const std::string& source_directory,
                                          const std::string& lib_directory,
                                          const std::string& optcfg_dir = "");

/**
 * Generates intermediate code for each source in `sources` and writes *.ir
 * outputs. Optionally interprets the code and prints any interpreted output to
 * stdout. If a source fails (I/O error, etc.), it reports to stdout and
 * continues to the next source.
 *
 * @param sources - the source files to type check.
 * @param out_directory - where to place generated diagnostic files.
 * @param source_directory - directory that the source files are in.
 * @param lib_directory - directory that the interface files are in.
 * @param interpret - interprets the generated intermediate code.
 */
void generate_ir_for_sources(const std::vector<std::string>& sources,
                             const std::string& out_directory,
                             const std::string& source_directory,
                             const std::string& lib_directory,
                             bool interpret);
}  // namespace ssy38::my_compiler
