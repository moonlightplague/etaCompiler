#include "CfgDotWriter.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <format>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace ssy38::my_compiler::cfg {
namespace {
namespace fs = std::filesystem;

/**
 * Helper: determines how to deal with special chars of a text in dot code
 *
 * @param text - input text to be rewritten.
 */
std::string dot_escape(const std::string& text) {
  std::string output;
  output.reserve(text.size());
  for (const char c : text) {
    switch (c) {
      case '\\':
        output += "\\\\";
        break;
      case '"':
        output += "\\\"";
        break;
      case '\n':
        output += "\\l";
        break;
      default:
        output += c;
        break;
    }
  }
  return output;
}

/**
 * Helper: determines the output edge labels of Cjump and Jump nodes
 *
 * @param type - node type: whether jump of Cjump.
 * @param branch_edge - whether the edge is true or false branch if it's a Cjump
 */
std::string edge_label(const stmt_type type, const bool branch_edge) {
  if (type == stmt_type::Cjump) {
    return branch_edge ? "true" : "false";
  }
  if (type == stmt_type::Jump) {
    return "jump";
  }
  return "";
}

/**
 * Helper: rewrite the original filename and function name as a component in
 * output filename
 *
 * @param input - input text to be rewritten.
 */
std::string sanitize_filename_component(const std::string& input) {
  std::string output;
  output.reserve(input.size());
  for (const char c : input) {
    if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' ||
        c == '.') {
      output.push_back(c);
    } else {
      output.push_back('_');
    }
  }
  return output.empty() ? "unnamed" : output;
}

/**
 * Helper: get the original function name from abi-encoded function name
 *
 * @param abi_name - abi-encoded function name.
 */
std::string decode_abi_function_name(const std::string& abi_name) {
  if (!abi_name.starts_with("_I")) {
    return abi_name;
  }

  std::string decoded;
  for (std::size_t i = 2; i < abi_name.size(); ++i) {
    const char c = abi_name[i];
    if (c == '_') {
      if (i + 1 < abi_name.size() && abi_name[i + 1] == '_') {
        decoded.push_back('_');
        ++i;
        continue;
      }
      return decoded.empty() ? abi_name : decoded;
    }
    if (c == '$' && i + 1 < abi_name.size() && abi_name[i + 1] == '1') {
      decoded.push_back('\'');
      ++i;
      continue;
    }
    decoded.push_back(c);
  }
  return decoded.empty() ? abi_name : decoded;
}

/**
 * Helper: determine the output path of the generated dot code
 *
 * @param directory - directory to output the generated dot code.
 * @param source_filename - the source filename of the input eta file.
 * @param function_name - the function name that owns the cfg.
 * @param phase - the current phase of cfg visualization.
 */
fs::path dot_output_path(const std::string& directory,
                         const std::string& source_filename,
                         const std::string& function_name,
                         const std::string& phase) {
  const fs::path input_path(source_filename.empty() ? "input.eta"
                                                    : source_filename);
  const fs::path dir_path =
      directory.empty() ? fs::path(".") : fs::path(directory);
  fs::path rel_parent = input_path.parent_path();
  if (input_path.is_absolute()) {
    rel_parent = input_path.parent_path().relative_path();
  }

  const std::string file_stem =
      sanitize_filename_component(input_path.stem().string());
  const std::string function_component =
      sanitize_filename_component(decode_abi_function_name(function_name));
  return dir_path / rel_parent /
         std::format("{}_{}_{}.dot", file_stem, function_component, phase);
}
}  // namespace

std::string graph_to_dot(const graph& graph, const std::string& function_name) {
  std::unordered_map<const node*, std::size_t> node_indices;
  node_indices.reserve(graph.node_list.size());
  for (std::size_t i = 0; i < graph.node_list.size(); ++i) {
    node_indices[graph.node_list[i].get()] = i;
  }

  std::ostringstream out;
  out << "digraph \"" << dot_escape(function_name) << "\" {\n";
  out << "  rankdir=TB;\n";
  out << "  node [shape=box, fontname=\"monospace\"];\n";

  for (std::size_t i = 0; i < graph.node_list.size(); ++i) {
    const auto& n = graph.node_list[i];
    std::string label = std::format("n{}: ", i);
    if (n->type == stmt_type::Start) {
      label += "START";
    } else if (n->stmt) {
      label += n->stmt->toString(0);
    } else {
      label += "<null>";
    }
    out << "  n" << i << " [label=\"" << dot_escape(label) << "\\l\"];\n";
  }

  for (std::size_t i = 0; i < graph.node_list.size(); ++i) {
    const auto& n = graph.node_list[i];
    const auto emit_edge = [&](const std::shared_ptr<node>& target,
                               const bool branch_edge) {
      if (!target) return;
      const auto target_index = node_indices.find(target.get());
      if (target_index == node_indices.end()) return;
      out << "  n" << i << " -> n" << target_index->second;
      const std::string label = edge_label(n->type, branch_edge);
      if (!label.empty()) {
        out << " [label=\"" << label << "\"]";
      }
      out << ";\n";
    };
    emit_edge(n->next, false);
    emit_edge(n->ifNode, true);
  }

  out << "}\n";
  return out.str();
}

void write_dot_for_phase(const graph& graph,
                         const std::vector<std::string>& requested_phases,
                         const std::string& output_directory,
                         const std::string& source_filename,
                         const std::string& function_name,
                         const std::string& phase) {
  if (std::find(requested_phases.begin(), requested_phases.end(), phase) ==
      requested_phases.end()) {
    return;
  }

  const fs::path output_path =
      dot_output_path(output_directory, source_filename, function_name, phase);
  if (const fs::path parent = output_path.parent_path(); !parent.empty()) {
    fs::create_directories(parent);
  }

  std::ofstream out(output_path, std::ios::binary);
  if (!out) {
    throw std::runtime_error(std::format("Unable to open file {} for writing",
                                         output_path.string()));
  }
  out << graph_to_dot(graph, function_name);
}

}  // namespace ssy38::my_compiler::cfg
