#pragma once

#include <string>
#include <vector>

#include "cfg.hpp"

namespace ssy38::my_compiler::cfg {

/**
 * Output a cfg's to a dot code owned by a function
 *
 * @param graph - the cfg to be visualized.
 * @param function_name - name of the function that owns the cfg.
 */
std::string graph_to_dot(const graph& graph, const std::string& function_name);

/**
 * Outputs the cfg to dot file whose name and directory is determined by several
 * variables
 *
 * @param graph - the cfg to be visualized.
 * @param requested_phases - the phases required by the user, if @param phase
 * does not match anything in requested_phases, do nothing
 * @param output_directory - the directory to output the visualization dot code.
 * @param source_filename - the name of the input eta file (determines the
 * filename of output dot file).
 * @param function_name - name of the function that owns the cfg (determines the
 * filename of output dot file).
 * @param phase - the current phase of cfg visualization (determines the
 * filename of output dot file).
 */
void write_dot_for_phase(const graph& graph,
                         const std::vector<std::string>& requested_phases,
                         const std::string& output_directory,
                         const std::string& source_filename,
                         const std::string& function_name,
                         const std::string& phase);

}  // namespace ssy38::my_compiler::cfg
