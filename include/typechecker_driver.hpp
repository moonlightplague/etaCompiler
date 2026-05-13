#pragma once

#include <string>
#include <vector>

#include "ast.hpp"

namespace ssy38::my_compiler {
/**
 * Runs semantic analysis for the specified source file and returns the AST of
 * the program. All callers must implement error handling for the possible
 * errors.
 *
 * @param source_filename - the source file to type check.
 * @param source_directory - directory that the source file is in.
 * @param lib_directory - directory that the interface files are in.
 * @return the typechecked AST program
 * @throws utils::EtaFrontendError if there's a lex/parse/typecheck error
 * @throws utils::EtaDriverError if there's an error typechecking an interface
 */
std::unique_ptr<ast::Program> typecheck_one_file(
    const std::string& source_filename, const std::string& source_directory,
    const std::string& lib_directory);

/**
 * Runs semantic analysis for each source in `sources` and writes *.typed
 * outputs. If a source fails (I/O error, etc.), it reports to stdout and
 * continues to the next source.
 *
 * @param sources - the source files to type check.
 * @param out_directory - where to place generated diagnostic files.
 * @param source_directory - directory that the source files are in.
 * @param lib_directory - directory that the interface files are in.
 */
void typecheck_sources(const std::vector<std::string>& sources,
                       const std::string& out_directory,
                       const std::string& source_directory,
                       const std::string& lib_directory);
}  // namespace ssy38::my_compiler