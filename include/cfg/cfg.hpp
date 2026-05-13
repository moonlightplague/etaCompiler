#pragma once

#include <immer/map.hpp>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include "copy.hpp"
#include "ir.hpp"

namespace ssy38::my_compiler::cfg {
enum class stmt_type {
  Assign,
  MemAssign,
  Call,
  Jump,
  Cjump,
  Return,
  Label,
  Start
};

struct node;

extern std::unordered_map<std::string, std::shared_ptr<node>> LABEL2NODE;
extern std::unordered_set<copy> COPIES;
extern std::unordered_set<std::string> TEMPS;
extern std::size_t ARG_COUNT;

/**
 * Helper for cp to calculate intersection between two copy relation sets
 *
 * @param A set A.
 * @param B set B.
 * @return An unordered_set of copy which is the intersection of A and B
 */
std::unordered_set<copy> copy_intersect(const std::unordered_set<copy> &A,
                                        const std::unordered_set<copy> &B);

/**
 * Helper for cp to calculate union between two copy relation sets
 *
 * @param A set A.
 * @param B set B.
 * @return An unordered_set of copy which is the union of A and B
 */
std::unordered_set<copy> copy_union(const std::unordered_set<copy> &A,
                                    const std::unordered_set<copy> &B);

/**
 * Helper for cp to calculate difference between two copy relation sets
 *
 * @param A set A.
 * @param B set B.
 * @return An unordered_set of copy which is A-B
 */
std::unordered_set<copy> copy_minus(const std::unordered_set<copy> &A,
                                    const std::unordered_set<copy> &B);

/**
 * Helper for cp to calculate canonical replacement of a temporary given copy
 * set A
 *
 * @param A Given copy set A.
 * @param t Input temporary name.
 * @return A canonical replacement temporary name of t
 */
std::string canon(const std::unordered_set<copy> &A, const std::string t);

struct node {
  std::shared_ptr<ir::Statement> stmt;
  stmt_type type;
  std::vector<std::shared_ptr<node>> pred;
  std::shared_ptr<node> next;
  std::shared_ptr<node> ifNode;
  std::unordered_set<std::string> in_dce;
  std::unordered_set<std::string> out_dce;
  bool unreachable;
  std::unordered_set<copy> in_cp;
  std::unordered_set<copy> out_cp;
  const_map in_ccp;
  const_map out_ccp;
  ir::vn::vnTable in_vn;
  node(std::shared_ptr<ir::Statement> stmt, stmt_type type)
      : stmt(stmt),
        type(type),
        pred({}),
        next(nullptr),
        ifNode(nullptr),
        in_dce(std::unordered_set<std::string>()),
        out_dce(std::unordered_set<std::string>()),
        in_cp(std::unordered_set<copy>(COPIES)),
        out_cp(std::unordered_set<copy>(COPIES)),
        unreachable(true),
        in_ccp(const_map()),
        out_ccp(const_map()),
        in_vn(ir::vn::vnTable()) {}
  /**
   * Extract all the used temporaries of the node
   *
   * @return An unordered_set including all the used temporaries
   */
  std::unordered_set<std::string> use();
  /**
   * Extract all the defined temporaries of the node
   *
   * @return An unordered_set including all the defined temporaries
   */
  std::unordered_set<std::string> def();
  /**
   * Extract all the generated copies of the node
   *
   * @return An unordered_set including all the generated copies
   */
  std::unordered_set<copy> gen();
  /**
   * Extract all the killed copies of the node
   *
   * @return An unordered_set including all the killed copies
   */
  std::unordered_set<copy> kill();
  /**
   * Rewrite the statement according to the copy set
   * @param A input copy set
   * @return
   */
  void rewrite(const std::unordered_set<copy> &A);
  /**
   * Transfer the const map when passing the node
   *
   * @param input input const map
   * @return the transferred const map
   */
  const_map ccp_transfer(const const_map &input);
  /**
   * Transfer the unreachable when passing the node
   *
   * @param n next node
   * @param input input const map
   * @return the transferred unreachable to be passed to next node
   */
  bool unreachable_transfer(std::shared_ptr<node> n);
  /**
   * Rewrite the statement according to the const map
   * @param A input const map
   * @return
   */
  void const_rewrite(const const_map &A);
  /**
   * Transfer the vnTable when passing the node
   *
   * @return the transferred vnTable to be passed to next node
   */
  ir::vn::vnTable vn_transfer(ir::vn::vnTable input);
};
struct graph {
  std::vector<std::shared_ptr<node>> node_list;
  graph(std::vector<std::shared_ptr<node>> node_list) : node_list(node_list) {}
  /**
   * Reconnect all the nodes currently in the graph, do everytime after
   * modifying the graph
   */
  void reconnect();
  /**
   * Remove all the currently-unreachable nodes in cfg
   * @return whether the node list has been structrually modified
   */
  bool unreachable_elim();
  /**
   * Execute dead code elimination optimization
   * @return whether the node list has been structrually modified
   */
  bool dead_code_elim();
  /**
   * Execute copy propagation optimization
   * @return whether the node list has been structrually modified
   */
  bool copy_propag();
  /**
   * Execute constant propagation optimization
   * @return
   */
  void const_propag();
  /**
   * Eliminate unused control-flow statements
   */
  void control_flow_elim();
  /**
   * Execute value numbering optimization
   * @return
   */
  void value_numbering();
};
}  // namespace ssy38::my_compiler::cfg
