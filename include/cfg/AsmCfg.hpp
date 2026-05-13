#pragma once
#include <memory>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "asm.hpp"

namespace ssy38::my_compiler::asm_cfg {
struct Node {
  std::shared_ptr<codegen::Command> command;

  std::vector<std::shared_ptr<Node>> predecessors;
  std::shared_ptr<Node>
      directSuccessor;  // primary successor, the immediate following node
  std::shared_ptr<Node>
      jumpSuccessor;  // jump target successor, null if node is not a jump node

  std::unordered_set<codegen::Register> live_in_registers;
  std::unordered_set<std::string> live_in_temporaries;

  std::unordered_set<codegen::Register>
      live_out_registers;                                // live-out registers
  std::unordered_set<std::string> live_out_temporaries;  // live-out temporaries

  Node(std::shared_ptr<codegen::Command> command)
      : command(std::move(command)) {}

  std::unordered_set<std::string> temporaries()
      const;  // all temporaries in a node

  std::pair<std::unordered_set<codegen::Register>,
            std::unordered_set<std::string>>
  use() const;
  std::pair<std::unordered_set<codegen::Register>,
            std::unordered_set<std::string>>
  def() const;
};

struct Graph {
  std::vector<std::shared_ptr<Node>> nodes;
  std::unordered_set<std::string> temporaries;  // all temporaries in a graph

  Graph(std::vector<std::shared_ptr<Node>> nodes) : nodes(std::move(nodes)) {
    reconnect();
  }

  Graph(std::vector<std::shared_ptr<codegen::Command>> commands) {
    nodes.reserve(commands.size());
    for (auto& command : commands) {
      nodes.push_back(std::make_shared<Node>(std::move(command)));
    }
    reconnect();
  }

  void reconnect();

  void liveness_analysis();

 private:
  std::unordered_map<std::string, std::shared_ptr<Node>>
      labelToNode;  // maps label names to nodes to construct the flow graph
};

struct GraphColoring {
  struct PairHash {
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2>& p) const {
      auto h1 = std::hash<T1>{}(p.first);
      auto h2 = std::hash<T2>{}(p.second);
      return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    }
  };

  using IGNode = std::variant<codegen::Register, std::string>;
  using Move = std::pair<IGNode, IGNode>;
  using Edge = std::pair<IGNode, IGNode>;
  using IGNodeSet = std::unordered_set<IGNode>;
  using IGNodeStack = std::vector<IGNode>;
  using MoveSet = std::unordered_set<Move, PairHash>;
  using EdgeSet = std::unordered_set<Edge, PairHash>;

  Graph cfg;
  IGNodeSet precolored = {
      codegen::Register::RAX, codegen::Register::RBX, codegen::Register::RCX,
      codegen::Register::RDX, codegen::Register::RSI, codegen::Register::RDI,
      codegen::Register::R8,  codegen::Register::R9,  codegen::Register::R10,
      codegen::Register::R11, codegen::Register::R12, codegen::Register::R13,
      codegen::Register::R14, codegen::Register::R15};

  // registers reserved for accessing stack locations
  IGNodeSet reserved = {codegen::Register::R10, codegen::Register::R11,
                        codegen::Register::RAX};
  int K = precolored.size() - reserved.size();

  EdgeSet adjSet;
  std::unordered_map<IGNode, IGNodeSet> adjList;
  std::unordered_map<IGNode, int> total_use;
  std::unordered_map<IGNode, int> total_def;
  std::unordered_map<IGNode, int> degree;
  std::unordered_map<IGNode, MoveSet> moveList;
  std::unordered_map<IGNode, IGNode> alias;
  std::unordered_map<IGNode, std::optional<codegen::Register>> color;

  struct SpilledNodeHeuristicCompare {
    GraphColoring& context;

    // minimize total use + total def / current degree
    inline double heuristic(const IGNode& a) const {
      return static_cast<double>(context.total_use[a] + context.total_def[a]) /
             static_cast<double>(context.degree[a]);
    }

    bool operator()(const IGNode& a, const IGNode& b) const {
      const double a_heuristic = heuristic(a);
      const double b_heuristic = heuristic(b);
      if (a_heuristic != b_heuristic) {
        return a_heuristic < b_heuristic;
      }
      return a < b;
    }
  };

  IGNodeSet initial;
  IGNodeSet simplifyWorklist;
  IGNodeSet freezeWorklist;
  IGNodeSet spillWorklist;
  std::set<IGNode, SpilledNodeHeuristicCompare> spilledNodes;
  IGNodeSet coalescedNodes;
  IGNodeSet coloredNodes;
  IGNodeStack selectStack;
  IGNodeSet selectStackSetView;

  MoveSet coalescedMoves;
  MoveSet constrainedMoves;
  MoveSet frozenMoves;
  MoveSet worklistMoves;
  MoveSet activeMoves;

  explicit GraphColoring(Graph cfg)
      : cfg(std::move(cfg)), spilledNodes(SpilledNodeHeuristicCompare{*this}) {
    for (auto temporary : this->cfg.temporaries) {
      initial.insert(temporary);
    }
  }

  void main();

  void build();

  void addEdge(IGNode u, IGNode v);

  void makeWorklist();

  IGNodeSet adjacent(IGNode n);

  MoveSet nodeMoves(IGNode n);

  bool moveRelated(IGNode n);

  void simplify();

  void decrementDegree(IGNode m);

  void enableMoves(IGNodeSet nodes);

  void addWorkList(IGNode u);

  bool ok(IGNode u, IGNode v);

  bool conservative(IGNode u, IGNode v);

  void coalesce();

  void combine(IGNode u, IGNode v);

  IGNode getAlias(IGNode n);

  void freeze();

  void freezeMoves(IGNode u);

  void selectSpill();

  void assignColors();
};

struct EtaGraphColoringRegisterAllocationVisitor
    : public visitor::EtaASMBaseVisitor {
  std::any visit(codegen::CommandSequence& node) override;
  std::any visit(codegen::ConstantOperand& node) override;
  std::any visit(codegen::RegisterOperand& node) override;
  std::any visit(codegen::TemporaryOperand& node) override;
  std::any visit(codegen::MemoryOperand& node) override;
  std::any visit(codegen::LabelOperand& node) override;
  std::any visit(codegen::Directive& node) override;
  std::any visit(codegen::LabelDeclaration& node) override;
  std::any visit(codegen::Operation& node) override;

 private:
  std::unordered_map<GraphColoring::IGNode, std::optional<codegen::Register>>
      color;
  std::unordered_map<std::string, int64_t>
      spilled_indices;  // spilled temporary name -> stack index (starting from
                        // 1)
  std::unordered_map<std::string, std::string>
      spill_aliases;  // temporary name -> coalesced representative used for
                      // stack slot sharing
  std::unordered_map<std::string, int64_t>
      shuttle_indices;  // spilled temporary name -> alloc_reg index (shuttle
                        // register index, starting from 1)
  int frame_size = 0;
};

}  // namespace ssy38::my_compiler::asm_cfg
