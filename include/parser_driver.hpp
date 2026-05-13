#pragma once

#include <string>
#include <vector>

namespace ssy38::my_compiler {

/**
 * Runs syntactic analysis for each source in `sources` and writes *.parsed
 * outputs. If a source fails (I/O error, etc.), it reports to stdout and
 * continues to the next source.
 *
 * @param sources - the source files to lex.
 * @param out_directory - where to place generated diagnostic files.
 * @param source_directory - directory that the source files are in.
 */
void parse_sources(const std::vector<std::string>& sources,
                   const std::string& out_directory,
                   const std::string& source_directory);

}  // namespace ssy38::my_compiler