#include "AsmCfg.hpp"

#include <algorithm>
#include <deque>
#include <queue>
#include <type_traits>

#include "cfg.hpp"
#include "utils.hpp"

namespace ssy38::my_compiler::asm_cfg {
namespace {
/**
 * Check if two operands are the same (for move coalescing purposes).
 *
 * @param lhs - pointer to the left hand operand.
 * @param rhs - pointer to the right hand operand.
 * @return whether the operands are the same.
 */
bool same_operand(const std::shared_ptr<codegen::Operand>& lhs,
                  const std::shared_ptr<codegen::Operand>& rhs) {
  if (lhs == rhs) return true;
  if (!lhs || !rhs) return false;
  if (const auto left =
          std::dynamic_pointer_cast<codegen::RegisterOperand>(lhs)) {
    const auto right = std::dynamic_pointer_cast<codegen::RegisterOperand>(rhs);
    return right && left->reg == right->reg;
  }
  return false;
}
}  // namespace

std::unordered_set<std::string> Node::temporaries() const {
  const auto operation_ptr =
      std::dynamic_pointer_cast<codegen::Operation>(command);
  if (!operation_ptr) {  // only operations have temporaries
    return {};
  }

  std::unordered_set<std::string> temporaries;

  auto collectFromOperand =
      [&temporaries](const std::shared_ptr<codegen::Operand>& operand) {
        if (const auto t =
                std::dynamic_pointer_cast<codegen::TemporaryOperand>(operand)) {
          temporaries.insert(t->name);
        } else if (const auto m =
                       std::dynamic_pointer_cast<codegen::MemoryOperand>(
                           operand)) {
          if (m->base) {
            if (const auto tmp =
                    std::get_if<codegen::TemporaryOperand>(&*m->base)) {
              temporaries.insert(tmp->name);
            }
          }
          if (m->idx) {
            if (const auto tmp =
                    std::get_if<codegen::TemporaryOperand>(&*m->idx)) {
              temporaries.insert(tmp->name);
            }
          }
        }
      };

  collectFromOperand(operation_ptr->operand1);
  collectFromOperand(operation_ptr->operand2);

  return temporaries;
}

std::pair<std::unordered_set<codegen::Register>,
          std::unordered_set<std::string>>
Node::use() const {
  const auto operation_ptr =
      std::dynamic_pointer_cast<codegen::Operation>(command);
  if (!operation_ptr) {  // only operations have operands
    return {{}, {}};
  }

  std::unordered_set<codegen::Register> use_registers;
  std::unordered_set<std::string> use_temps;

  auto collectFromMemoryOperand =
      [&use_registers,
       &use_temps](const std::shared_ptr<codegen::MemoryOperand>& m) {
        if (m->base) {
          if (const auto* reg =
                  std::get_if<codegen::RegisterOperand>(&*m->base)) {
            use_registers.insert(reg->reg);
          } else if (const auto* temp =
                         std::get_if<codegen::TemporaryOperand>(&*m->base)) {
            use_temps.insert(temp->name);
          }
        }
        if (m->idx) {
          if (const auto* reg =
                  std::get_if<codegen::RegisterOperand>(&*m->idx)) {
            use_registers.insert(reg->reg);
          } else if (const auto* temp =
                         std::get_if<codegen::TemporaryOperand>(&*m->idx)) {
            use_temps.insert(temp->name);
          }
        }
      };

  auto collectFromOperand =
      [&use_registers, &use_temps, collectFromMemoryOperand,
       operation_ptr](const std::shared_ptr<codegen::Operand>& operand) {
        if (const auto r =
                std::dynamic_pointer_cast<codegen::RegisterOperand>(operand)) {
          // special instructions use 8-bit registers
          if (operation_ptr->opcode == codegen::OpCode::SHR ||
              operation_ptr->opcode == codegen::OpCode::SHL ||
              operation_ptr->opcode == codegen::OpCode::SAR ||
              operation_ptr->opcode == codegen::OpCode::SETE ||
              operation_ptr->opcode == codegen::OpCode::SETNE ||
              operation_ptr->opcode == codegen::OpCode::SETL ||
              operation_ptr->opcode == codegen::OpCode::SETLE ||
              operation_ptr->opcode == codegen::OpCode::SETG ||
              operation_ptr->opcode == codegen::OpCode::SETGE ||
              operation_ptr->opcode == codegen::OpCode::SETB) {
            use_registers.insert(codegen::promote_to_64(r->reg));
          } else {
            use_registers.insert(r->reg);
          }
        } else if (const auto t =
                       std::dynamic_pointer_cast<codegen::TemporaryOperand>(
                           operand)) {
          use_temps.insert(t->name);
        } else if (const auto m =
                       std::dynamic_pointer_cast<codegen::MemoryOperand>(
                           operand)) {
          collectFromMemoryOperand(m);
        }
      };

  // Check first operand
  // only certain instructions use the first operand
  if (operation_ptr->opcode != codegen::OpCode::MOV &&
      operation_ptr->opcode != codegen::OpCode::POP &&
      operation_ptr->opcode != codegen::OpCode::LEA &&
      operation_ptr->opcode != codegen::OpCode::SETE &&
      operation_ptr->opcode != codegen::OpCode::SETNE &&
      operation_ptr->opcode != codegen::OpCode::SETL &&
      operation_ptr->opcode != codegen::OpCode::SETLE &&
      operation_ptr->opcode != codegen::OpCode::SETG &&
      operation_ptr->opcode != codegen::OpCode::SETGE &&
      operation_ptr->opcode != codegen::OpCode::SETB) {
    collectFromOperand(operation_ptr->operand1);
  } else {  // memory operands are always used
    if (const auto m = std::dynamic_pointer_cast<codegen::MemoryOperand>(
            operation_ptr->operand1)) {
      collectFromMemoryOperand(m);
    }
  }

  // Check second operand, all operands are used
  collectFromOperand(operation_ptr->operand2);

  // account for implicitly used registers
  if (operation_ptr->opcode == codegen::OpCode::IDIV) {
    use_registers.insert(codegen::Register::RDX);
    use_registers.insert(codegen::Register::RAX);
  } else if (operation_ptr->opcode == codegen::OpCode::CQO) {
    use_registers.insert(codegen::Register::RAX);
  } else if (operation_ptr->opcode == codegen::OpCode::IMUL &&
             operation_ptr->operand2 ==
                 nullptr) {  // only uses implicit registers in one-operand form
    use_registers.insert(codegen::Register::RAX);
  } else if (operation_ptr->opcode == codegen::OpCode::RET) {
    if (operation_ptr->numTerms.value() >= 1) {
      use_registers.insert(codegen::Register::RAX);
    }
    if (operation_ptr->numTerms.value() >= 2) {
      use_registers.insert(codegen::Register::RDX);
    }
  } else if (operation_ptr->opcode == codegen::OpCode::CALL) {
    if (operation_ptr->numTerms.value() >= 1) {
      use_registers.insert(codegen::Register::RDI);
    }
    if (operation_ptr->numTerms.value() >= 2) {
      use_registers.insert(codegen::Register::RSI);
    }
    if (operation_ptr->numTerms.value() >= 3) {
      use_registers.insert(codegen::Register::RDX);
    }
    if (operation_ptr->numTerms.value() >= 4) {
      use_registers.insert(codegen::Register::RCX);
    }
    if (operation_ptr->numTerms.value() >= 5) {
      use_registers.insert(codegen::Register::R8);
    }
    if (operation_ptr->numTerms.value() >= 6) {
      use_registers.insert(codegen::Register::R9);
    }
  }

  return {use_registers, use_temps};
}

std::pair<std::unordered_set<codegen::Register>,
          std::unordered_set<std::string>>
Node::def() const {
  const auto operation_ptr =
      std::dynamic_pointer_cast<codegen::Operation>(command);
  if (!operation_ptr) {  // only operations have operands
    return {{}, {}};
  }

  std::unordered_set<codegen::Register> def_registers;
  std::unordered_set<std::string> def_temps;

  if (operation_ptr->opcode == codegen::OpCode::MOV ||
      operation_ptr->opcode == codegen::OpCode::POP ||
      operation_ptr->opcode == codegen::OpCode::LEA ||
      operation_ptr->opcode == codegen::OpCode::ADD ||
      operation_ptr->opcode == codegen::OpCode::SUB ||
      operation_ptr->opcode == codegen::OpCode::IMUL ||
      operation_ptr->opcode == codegen::OpCode::AND ||
      operation_ptr->opcode == codegen::OpCode::OR ||
      operation_ptr->opcode == codegen::OpCode::XOR ||
      operation_ptr->opcode == codegen::OpCode::SHL ||
      operation_ptr->opcode == codegen::OpCode::SHR ||
      operation_ptr->opcode == codegen::OpCode::SAR ||
      operation_ptr->opcode == codegen::OpCode::NEG ||
      operation_ptr->opcode == codegen::OpCode::SETE ||
      operation_ptr->opcode == codegen::OpCode::SETNE ||
      operation_ptr->opcode == codegen::OpCode::SETL ||
      operation_ptr->opcode == codegen::OpCode::SETG ||
      operation_ptr->opcode == codegen::OpCode::SETB ||
      operation_ptr->opcode == codegen::OpCode::SETGE ||
      operation_ptr->opcode == codegen::OpCode::SETLE) {
    if (const auto r = std::dynamic_pointer_cast<codegen::RegisterOperand>(
            operation_ptr->operand1)) {
      def_registers.insert(r->reg);
    } else if (const auto t =
                   std::dynamic_pointer_cast<codegen::TemporaryOperand>(
                       operation_ptr->operand1)) {
      def_temps.insert(t->name);
    }
  }

  // account for implicitly defined registers
  if (operation_ptr->opcode == codegen::OpCode::IDIV) {
    def_registers.insert(codegen::Register::RDX);
    def_registers.insert(codegen::Register::RAX);
  } else if (operation_ptr->opcode == codegen::OpCode::CQO) {
    def_registers.insert(codegen::Register::RDX);
  } else if (operation_ptr->opcode == codegen::OpCode::IMUL &&
             operation_ptr->operand2 ==
                 nullptr) {  // only defines implicit registers in one-operand
    // form
    def_registers.insert(codegen::Register::RDX);
    def_registers.insert(codegen::Register::RAX);
  } else if (operation_ptr->opcode == codegen::OpCode::CALL) {
    def_registers.insert({codegen::Register::RAX, codegen::Register::RCX,
                          codegen::Register::RDX, codegen::Register::RSI,
                          codegen::Register::RDI, codegen::Register::R8,
                          codegen::Register::R9, codegen::Register::R10,
                          codegen::Register::R11});
  }

  return {def_registers, def_temps};
}

void Graph::reconnect() {
  labelToNode.clear();
  temporaries.clear();
  // first pass: reset each node's edges, populate label map and temporaries set
  for (const auto node : nodes) {
    node->predecessors.clear();
    node->directSuccessor = nullptr;
    node->jumpSuccessor = nullptr;
    if (const auto label_ptr =
            std::dynamic_pointer_cast<codegen::LabelDeclaration>(
                node->command)) {
      labelToNode[label_ptr->name] = node;
    }
    const auto node_temporaries = node->temporaries();
    temporaries.insert(node_temporaries.begin(), node_temporaries.end());
  }
  // second pass: connect edges between nodes
  for (size_t i = 0; i < nodes.size(); ++i) {
    const auto node = nodes[i];

    if (i < nodes.size() - 1) {
      node->directSuccessor = nodes[i + 1];
    }
    // direct predecessor
    if (i > 0) {
      node->predecessors.push_back(nodes[i - 1]);
    }

    // jump and return
    if (const auto operation_ptr =
            std::dynamic_pointer_cast<codegen::Operation>(node->command)) {
      if (operation_ptr->opcode == codegen::OpCode::JB ||
          operation_ptr->opcode == codegen::OpCode::JG ||
          operation_ptr->opcode == codegen::OpCode::JGE ||
          operation_ptr->opcode == codegen::OpCode::JL ||
          operation_ptr->opcode == codegen::OpCode::JLE ||
          operation_ptr->opcode == codegen::OpCode::JMP ||
          operation_ptr->opcode == codegen::OpCode::JNE ||
          operation_ptr->opcode == codegen::OpCode::JNS ||
          operation_ptr->opcode == codegen::OpCode::JNZ ||
          operation_ptr->opcode == codegen::OpCode::JS ||
          operation_ptr->opcode == codegen::OpCode::JZ) {
        const auto label = std::dynamic_pointer_cast<codegen::LabelOperand>(
            operation_ptr->operand1);
        if (!label) {
          throw std::logic_error("Jump instruction needs to have LabelOperand");
        }
        node->jumpSuccessor = labelToNode[label->name];
        node->jumpSuccessor->predecessors.push_back(node);
        if (operation_ptr->opcode == codegen::OpCode::JMP) {
          node->directSuccessor = node->jumpSuccessor;
        }
      } else if (operation_ptr->opcode == codegen::OpCode::RET) {
        node->directSuccessor = nullptr;
      }
    }
  }
}

void Graph::liveness_analysis() {
  std::queue<std::shared_ptr<Node>> worklist;
  std::unordered_set<Node*> in_worklist;
  for (const auto& node : nodes) {
    node->live_in_registers.clear();
    node->live_out_registers.clear();
    node->live_in_temporaries.clear();
    node->live_out_temporaries.clear();

    worklist.push(node);
    in_worklist.insert(node.get());
  }

  while (!worklist.empty()) {
    auto node = worklist.front();
    worklist.pop();
    in_worklist.erase(node.get());

    // out[n] = union of in[n'] for each successor n'
    std::unordered_set<codegen::Register> new_out_regs;
    std::unordered_set<std::string> new_out_temps;
    auto absorb = [&](const std::shared_ptr<Node>& succ) {
      if (!succ) return;
      new_out_regs.insert(succ->live_in_registers.begin(),
                          succ->live_in_registers.end());
      new_out_temps.insert(succ->live_in_temporaries.begin(),
                           succ->live_in_temporaries.end());
    };
    absorb(node->directSuccessor);
    if (node->jumpSuccessor && node->jumpSuccessor != node->directSuccessor) {
      absorb(node->jumpSuccessor);
    }

    // in[n] = use(n) ∪ (out[n] − def(n))
    auto [use_regs, use_temps] = node->use();
    auto [def_regs, def_temps] = node->def();

    auto new_in_regs = use_regs;
    for (const auto& r : new_out_regs) {
      if (!def_regs.contains(r)) new_in_regs.insert(r);
    }
    auto new_in_temps = use_temps;
    for (const auto& t : new_out_temps) {
      if (!def_temps.contains(t)) new_in_temps.insert(t);
    }

    const bool changed = new_out_regs != node->live_out_registers ||
                         new_out_temps != node->live_out_temporaries ||
                         new_in_regs != node->live_in_registers ||
                         new_in_temps != node->live_in_temporaries;

    if (changed) {
      node->live_out_registers = std::move(new_out_regs);
      node->live_out_temporaries = std::move(new_out_temps);
      node->live_in_registers = std::move(new_in_regs);
      node->live_in_temporaries = std::move(new_in_temps);

      for (const auto& pred : node->predecessors) {
        if (pred && in_worklist.insert(pred.get())
                        .second) {  // predecessor not in worklist
          worklist.push(pred);
        }
      }
    }
  }
}

void GraphColoring::main() {
  build();
  makeWorklist();
  while (!(simplifyWorklist.empty() && worklistMoves.empty() &&
           freezeWorklist.empty() && spillWorklist.empty())) {
    if (!simplifyWorklist.empty()) {
      simplify();
    } else if (!worklistMoves.empty()) {
      coalesce();
    } else if (!freezeWorklist.empty()) {
      freeze();
    } else if (!spillWorklist.empty()) {
      selectSpill();
    }
  }
  assignColors();
}

GraphColoring::IGNodeSet liveOut(std::shared_ptr<Node> node) {
  GraphColoring::IGNodeSet set;
  for (auto reg : node->live_out_registers) set.insert(reg);
  for (auto temp : node->live_out_temporaries) set.insert(temp);
  return set;
}

GraphColoring::IGNodeSet def(std::shared_ptr<Node> node) {
  GraphColoring::IGNodeSet set;
  auto [regs, temps] = node->def();
  for (auto reg : regs) set.insert(reg);
  for (auto temp : temps) set.insert(temp);
  return set;
}

// returns a representation of the instruction node in the graph coloring
// algorithm if the node is a move
std::optional<GraphColoring::Move> isMoveInstruction(
    std::shared_ptr<Node> node) {
  if (auto command = dynamic_pointer_cast<codegen::Operation>(node->command);
      command && command->opcode == codegen::OpCode::MOV) {
    auto t1 =
        dynamic_pointer_cast<codegen::TemporaryOperand>(command->operand1);
    auto t2 =
        dynamic_pointer_cast<codegen::TemporaryOperand>(command->operand2);
    if (t1 && t2) {
      return GraphColoring::Move{t1->name, t2->name};
    }
    return {};
  }
  return {};
}

void GraphColoring::build() {
  for (auto n : precolored) {
    color[n] = *std::get_if<codegen::Register>(&n);
  }
  for (auto n : initial) {
    if (std::holds_alternative<std::string>(n)) {
      degree[n] = 0;
      total_use[n] = 0;
      total_def[n] = 0;
    } else {
      color[n] = *std::get_if<codegen::Register>(&n);
    }
  }
  for (auto instruction : cfg.nodes) {
    for (auto n : instruction->use().second) total_use[n]++;
    for (auto n : instruction->def().second) total_def[n]++;
    auto live = liveOut(instruction);
    auto moveOpt = isMoveInstruction(instruction);
    if (moveOpt) {
      auto move = *moveOpt;
      live.erase(move.second);
      moveList[move.first].insert(move);
      moveList[move.second].insert(move);
      worklistMoves.insert(move);
    }
    auto defI = def(instruction);
    live.insert(defI.begin(), defI.end());
    for (auto d : defI) {
      for (auto l : live) {
        addEdge(l, d);
      }
    }
  }
}

void GraphColoring::addEdge(IGNode u, IGNode v) {
  if (!adjSet.contains({u, v}) && u != v) {
    adjSet.insert({u, v});
    adjSet.insert({v, u});
    if (std::holds_alternative<std::string>(u)) {
      adjList[u].insert(v);
      if (spillWorklist.contains(u)) {
        spillWorklist.erase(u);
        degree[u]++;
        spillWorklist.insert(u);
      } else
        degree[u]++;
    }
    if (std::holds_alternative<std::string>(v)) {
      adjList[v].insert(u);
      if (spillWorklist.contains(v)) {
        spillWorklist.erase(v);
        degree[v]++;
        spillWorklist.insert(v);
      } else
        degree[v]++;
    }
  }
}

void GraphColoring::makeWorklist() {
  for (auto n : initial) {
    if (degree[n] >= K)
      spillWorklist.insert(n);
    else if (moveRelated(n))
      freezeWorklist.insert(n);
    else
      simplifyWorklist.insert(n);
  }
  initial = {};
}

GraphColoring::IGNodeSet GraphColoring::adjacent(IGNode n) {
  IGNodeSet set;
  for (auto neighbor : adjList[n])
    if (!selectStackSetView.contains(neighbor) &&
        !coalescedNodes.contains(neighbor))
      set.insert(neighbor);
  return set;
}

GraphColoring::MoveSet GraphColoring::nodeMoves(IGNode n) {
  MoveSet set;
  for (auto move : moveList[n])
    if (activeMoves.contains(move) || worklistMoves.contains(move))
      set.insert(move);
  return set;
}

bool GraphColoring::moveRelated(IGNode n) { return !nodeMoves(n).empty(); }

void GraphColoring::simplify() {
  auto n = simplifyWorklist.extract(simplifyWorklist.begin()).value();
  selectStack.push_back(n);
  selectStackSetView.insert(n);
  for (auto m : adjacent(n)) decrementDegree(m);
}

void GraphColoring::decrementDegree(IGNode m) {
  int d = degree[m];
  if (spillWorklist.contains(m)) {
    spillWorklist.erase(m);
    degree[m]--;
    spillWorklist.insert(m);
  } else
    degree[m]--;
  if (d == K) {
    auto toBeEnabled = adjacent(m);
    toBeEnabled.insert(m);
    enableMoves(toBeEnabled);
    spillWorklist.erase(m);
    if (moveRelated(m))
      freezeWorklist.insert(m);
    else
      simplifyWorklist.insert(m);
  }
}

void GraphColoring::enableMoves(IGNodeSet nodes) {
  for (auto n : nodes) {
    for (auto move : nodeMoves(n)) {
      if (activeMoves.contains(move)) {
        activeMoves.erase(move);
        worklistMoves.insert(move);
      }
    }
  }
}

void GraphColoring::addWorkList(IGNode u) {
  if (std::holds_alternative<std::string>(u) && !(moveRelated(u)) &&
      degree[u] < K) {
    freezeWorklist.erase(u);
    simplifyWorklist.insert(u);
  }
}

bool GraphColoring::ok(IGNode u, IGNode v) {
  for (auto t : adjacent(v)) {
    if (!(degree[t] < K || std::holds_alternative<codegen::Register>(t) ||
          adjSet.contains({t, u})))
      return false;
  }
  return true;
}

bool GraphColoring::conservative(IGNode u, IGNode v) {
  auto nodes = adjacent(u);
  auto adjacent_v = adjacent(v);
  nodes.insert(adjacent_v.begin(), adjacent_v.end());
  int k = 0;
  for (auto n : nodes) {
    if (degree[n] >= K) k++;
  }
  return k < K;
}

void GraphColoring::coalesce() {
  auto m = worklistMoves.extract(worklistMoves.begin()).value();
  auto [x, y] = m;
  x = getAlias(x);
  y = getAlias(y);
  IGNode u, v;
  if (std::holds_alternative<codegen::Register>(y)) {
    u = y;
    v = x;
  } else {
    u = x;
    v = y;
  }
  if (u == v) {
    coalescedMoves.insert(m);
    addWorkList(u);
  } else if (std::holds_alternative<codegen::Register>(v) ||
             adjSet.contains({u, v})) {
    constrainedMoves.insert({u, v});
    addWorkList(u);
    addWorkList(v);
  } else if ((std::holds_alternative<codegen::Register>(u) && ok(u, v)) ||
             (std::holds_alternative<std::string>(u) && conservative(u, v))) {
    coalescedMoves.insert(m);
    combine(u, v);
    addWorkList(u);
  } else {
    activeMoves.insert(m);
  }
}

void GraphColoring::combine(IGNode u, IGNode v) {
  if (freezeWorklist.contains(v))
    freezeWorklist.erase(v);
  else
    spillWorklist.erase(v);
  coalescedNodes.insert(v);
  alias[v] = u;
  moveList[u].insert(moveList[v].begin(), moveList[v].end());
  enableMoves({v});
  for (auto t : adjacent(v)) {
    addEdge(t, u);
    decrementDegree(t);
  }
  if (degree[u] >= K && freezeWorklist.contains(u)) {
    freezeWorklist.erase(u);
    spillWorklist.insert(u);
  }
}

GraphColoring::IGNode GraphColoring::getAlias(IGNode n) {
  if (coalescedNodes.contains(n)) {
    auto resolved = getAlias(alias.at(n));
    alias[n] = resolved;
    return resolved;
  }
  return n;
}

void GraphColoring::freeze() {
  auto u = freezeWorklist.extract(freezeWorklist.begin()).value();
  simplifyWorklist.insert(u);
  freezeMoves(u);
}

void GraphColoring::freezeMoves(IGNode u) {
  for (auto m : nodeMoves(u)) {
    auto [x, y] = m;
    IGNode v = getAlias(y) == getAlias(u) ? getAlias(x) : getAlias(y);
    activeMoves.erase(m);
    frozenMoves.insert(m);
    if (freezeWorklist.contains(v) && !moveRelated(v)) {
      freezeWorklist.erase(v);
      simplifyWorklist.insert(v);
    }
  }
}

void GraphColoring::selectSpill() {
  auto m = spillWorklist.extract(spillWorklist.begin()).value();
  simplifyWorklist.insert(m);
  freezeMoves(m);
}

void GraphColoring::assignColors() {
  while (!selectStack.empty()) {
    auto n = selectStack.back();
    selectStack.pop_back();
    auto okColors = precolored;
    for (const auto& reg : reserved) okColors.erase(reg);
    for (auto w : adjList[n]) {
      auto alias = getAlias(w);
      if (precolored.contains(alias)) {
        okColors.erase(alias);
      } else if (coloredNodes.contains(alias)) {
        auto w_color = color[alias];
        if (w_color.has_value()) {
          okColors.erase(w_color.value());
        }
      }
    }
    if (okColors.empty()) {
      spilledNodes.insert(n);
      color[n] = std::nullopt;
    } else {
      coloredNodes.insert(n);
      auto c = *okColors.begin();
      color[n] = *std::get_if<codegen::Register>(&c);
    }
  }
  for (auto n : coalescedNodes) color[n] = color[getAlias(n)];
}

inline codegen::Register alloc_reg(const int i) {
  if (i == 1) {
    return codegen::Register::R10;
  } else if (i == 2) {
    return codegen::Register::R11;
  } else if (i == 3) {
    return codegen::Register::RAX;
  } else {
    throw std::runtime_error("Each operation should only have <= 3 registers");
  }
}

std::any EtaGraphColoringRegisterAllocationVisitor::visit(
    codegen::CommandSequence& node) {
  std::vector<std::shared_ptr<codegen::Command>> results;

  std::vector<std::shared_ptr<codegen::Command>> current_function;
  auto frame_size_bytes = [](const int64_t slot_count) {
    return slot_count % 2 == 0 ? slot_count * 8 : slot_count * 8 + 8;
  };
  for (const auto& command : node.commands) {
    if (auto l =
            std::dynamic_pointer_cast<codegen::LabelDeclaration>(command)) {
      if (l->name == "END_OF_FUNC") {
        // Perform live variable analysis and graph coloring register allocation
        Graph cfg(current_function);
        cfg.liveness_analysis();
        GraphColoring graph_coloring(cfg);
        graph_coloring.main();
        color = graph_coloring.color;
        spilled_indices.clear();
        spill_aliases.clear();
        for (const auto& temp : cfg.temporaries) {
          const auto alias =
              graph_coloring.getAlias(GraphColoring::IGNode{temp});
          if (const auto* alias_temp = std::get_if<std::string>(&alias)) {
            spill_aliases[temp] = *alias_temp;
          } else {
            spill_aliases[temp] = temp;
          }
        }

        // Rewrite first: this discovers the concrete spill slots used after
        // coalesced aliases have been folded onto shared stack locations.
        std::vector<std::shared_ptr<codegen::Command>> rewritten;
        for (const auto& current_function_command : current_function) {
          auto sequence = std::any_cast<codegen::CommandSequence>(
              current_function_command->accept(*this));
          rewritten.insert(rewritten.end(), sequence.commands.begin(),
                           sequence.commands.end());
        }
        frame_size = spilled_indices.size();
        for (const auto& rewritten_command : rewritten) {
          if (auto op = std::dynamic_pointer_cast<codegen::Operation>(
                  rewritten_command);
              op && op->opcode == codegen::OpCode::ENTER) {
            op->operand1 = std::make_shared<codegen::ConstantOperand>(
                frame_size_bytes(frame_size));
            break;
          }
        }
        results.insert(results.end(), rewritten.begin(), rewritten.end());

        // move onto next function
        current_function.clear();
        spilled_indices.clear();
        spill_aliases.clear();
      } else {
        current_function.push_back(l);
      }
    } else {
      current_function.push_back(command);
    }
  }
  // footer section
  for (const auto& current_function_command : current_function) {
    auto sequence = std::any_cast<codegen::CommandSequence>(
        current_function_command->accept(*this));
    results.insert(results.end(), sequence.commands.begin(),
                   sequence.commands.end());
  }
  current_function.clear();

  return codegen::CommandSequence(results);
}

std::any EtaGraphColoringRegisterAllocationVisitor::visit(
    codegen::ConstantOperand& node) {
  return node;
}

std::any EtaGraphColoringRegisterAllocationVisitor::visit(
    codegen::RegisterOperand& node) {
  return node;
}

std::any EtaGraphColoringRegisterAllocationVisitor::visit(
    codegen::LabelOperand& node) {
  return node;
}

std::any EtaGraphColoringRegisterAllocationVisitor::visit(
    codegen::TemporaryOperand& node) {
  auto it = color.find(GraphColoring::IGNode{node.name});
  if (it != color.end() && it->second.has_value()) {
    return codegen::RegisterOperand(it->second.value());
  } else {
    return codegen::RegisterOperand(alloc_reg(shuttle_indices[node.name]));
  }
}

std::any EtaGraphColoringRegisterAllocationVisitor::visit(
    codegen::MemoryOperand& node) {
  std::optional<std::variant<codegen::ConstantOperand, codegen::RegisterOperand,
                             codegen::TemporaryOperand, codegen::LabelOperand>>
      translatedBase;
  if (node.base.has_value()) {
    translatedBase = std::visit(
        utils::overloaded{
            [](codegen::ConstantOperand& operand) {
              return std::variant<
                  codegen::ConstantOperand, codegen::RegisterOperand,
                  codegen::TemporaryOperand, codegen::LabelOperand>(operand);
            },
            [](codegen::RegisterOperand& operand) {
              return std::variant<
                  codegen::ConstantOperand, codegen::RegisterOperand,
                  codegen::TemporaryOperand, codegen::LabelOperand>(operand);
            },
            [this](codegen::TemporaryOperand& operand) {
              return std::variant<
                  codegen::ConstantOperand, codegen::RegisterOperand,
                  codegen::TemporaryOperand, codegen::LabelOperand>(
                  std::any_cast<codegen::RegisterOperand>(
                      operand.accept(*this)));
            },
            [](codegen::LabelOperand& operand) {
              return std::variant<
                  codegen::ConstantOperand, codegen::RegisterOperand,
                  codegen::TemporaryOperand, codegen::LabelOperand>(operand);
            }},
        node.base.value());
  } else {
    translatedBase = std::nullopt;
  }

  std::optional<
      std::variant<codegen::RegisterOperand, codegen::TemporaryOperand>>
      translatedIdx;
  if (node.idx.has_value()) {
    translatedIdx = std::visit(
        utils::overloaded{[](codegen::RegisterOperand& operand) {
                            return std::variant<codegen::RegisterOperand,
                                                codegen::TemporaryOperand>(
                                operand);
                          },
                          [this](codegen::TemporaryOperand& operand) {
                            return std::variant<codegen::RegisterOperand,
                                                codegen::TemporaryOperand>(
                                std::any_cast<codegen::RegisterOperand>(
                                    operand.accept(*this)));
                          }},
        node.idx.value());
  } else {
    translatedIdx = std::nullopt;
  }

  return codegen::MemoryOperand(node.size, translatedBase, translatedIdx,
                                node.scale, node.constant);
}

std::any EtaGraphColoringRegisterAllocationVisitor::visit(
    codegen::Directive& node) {
  return codegen::CommandSequence({std::make_shared<codegen::Directive>(node)});
}

std::any EtaGraphColoringRegisterAllocationVisitor::visit(
    codegen::LabelDeclaration& node) {
  if (node.name == "END_OF_FUNC") {
    // if the node is an END_OF_FUNC flag, delete itself.
    return codegen::CommandSequence({});
  }
  return codegen::CommandSequence(
      {std::make_shared<codegen::LabelDeclaration>(node)});
}

std::any EtaGraphColoringRegisterAllocationVisitor::visit(
    codegen::Operation& node) {
  shuttle_indices.clear();
  if (node.opcode == codegen::OpCode::ENTER) {
    node.operand1 = std::make_shared<codegen::ConstantOperand>(
        frame_size % 2 == 0 ? frame_size * 8 : frame_size * 8 + 8);
    return codegen::CommandSequence(
        {std::make_shared<codegen::Operation>(node)});
  }

  // check each operand in the operation

  // check if temp is spilled to stack
  std::vector<codegen::TemporaryOperand>
      spilled_operands;  // temps that are spilled and used in this operation
  auto add_temporary_if_needed =
      [&spilled_operands](const codegen::TemporaryOperand& temp) {
        const bool exists =
            std::any_of(spilled_operands.begin(), spilled_operands.end(),
                        [&temp](const codegen::TemporaryOperand& item) {
                          return item.name == temp.name;
                        });
        if (!exists) {
          spilled_operands.push_back(temp);
        }
      };
  auto stack_slot_key = [this](const std::string& temp_name) {
    if (const auto alias = spill_aliases.find(temp_name);
        alias != spill_aliases.end()) {
      return alias->second;
    }
    return temp_name;
  };
  auto add_to_stack = [&](const codegen::TemporaryOperand& temp) {
    const auto key = stack_slot_key(temp.name);
    if (spilled_indices.contains(key)) {
      add_temporary_if_needed(temp);
      return;
    }

    auto it = color.find(GraphColoring::IGNode{temp.name});
    if (it == color.end() || !it->second.has_value()) {  // temp is spilled
      spilled_indices[key] = spilled_indices.size() + 1;
      add_temporary_if_needed(temp);
    }
  };

  for (const auto& operand : {node.operand1, node.operand2}) {
    if (operand) {
      if (const auto temp_ptr =
              std::dynamic_pointer_cast<codegen::TemporaryOperand>(operand)) {
        add_to_stack(*temp_ptr);
      } else if (const auto mem_ptr =
                     std::dynamic_pointer_cast<codegen::MemoryOperand>(
                         operand)) {
        if (mem_ptr->base.has_value()) {
          if (const auto base_temp = std::get_if<codegen::TemporaryOperand>(
                  &mem_ptr->base.value())) {
            add_to_stack(*base_temp);
          }
        }
        if (mem_ptr->idx.has_value()) {
          if (const auto idx_temp = std::get_if<codegen::TemporaryOperand>(
                  &mem_ptr->idx.value())) {
            add_to_stack(*idx_temp);
          }
        }
      }
    }
  }

  if (spilled_operands.size() > 3) {
    throw std::runtime_error(
        "Each operation should only have <= 3 spilled temporary operands");
  }

  const auto [_, defined_temps] =
      Node(std::make_shared<codegen::Operation>(node)).def();

  std::vector<std::shared_ptr<codegen::Command>> results;

  // shuttle from stack to registers
  for (const auto& temp : spilled_operands) {
    if (!shuttle_indices.contains(temp.name)) {
      shuttle_indices[temp.name] = shuttle_indices.size() + 1;
    }
    results.emplace_back(std::make_shared<codegen::Operation>(
        codegen::OpCode::MOV,
        std::make_shared<codegen::RegisterOperand>(
            alloc_reg(shuttle_indices[temp.name])),
        std::make_shared<codegen::MemoryOperand>(
            codegen::Size::QWORD_PTR,
            codegen::RegisterOperand(codegen::Register::RBP), std::nullopt,
            std::nullopt,
            codegen::ConstantOperand(
                -8 * spilled_indices.at(stack_slot_key(temp.name))))));
  }

  // translate the operation (replace temporaries with registers)
  auto translate_operand =
      [this](const std::shared_ptr<codegen::Operand>& operand)
      -> std::shared_ptr<codegen::Operand> {
    if (!operand) {
      return nullptr;
    }
    if (const auto constant_ptr =
            std::dynamic_pointer_cast<codegen::ConstantOperand>(operand)) {
      return std::make_shared<codegen::ConstantOperand>(*constant_ptr);
    }
    if (const auto register_ptr =
            std::dynamic_pointer_cast<codegen::RegisterOperand>(operand)) {
      return std::make_shared<codegen::RegisterOperand>(*register_ptr);
    }
    if (const auto temporary_ptr =
            std::dynamic_pointer_cast<codegen::TemporaryOperand>(operand)) {
      return std::make_shared<codegen::RegisterOperand>(
          std::any_cast<codegen::RegisterOperand>(
              temporary_ptr->accept(*this)));
    }
    if (const auto memory_ptr =
            std::dynamic_pointer_cast<codegen::MemoryOperand>(operand)) {
      return std::make_shared<codegen::MemoryOperand>(
          std::any_cast<codegen::MemoryOperand>(memory_ptr->accept(*this)));
    }
    if (const auto label_ptr =
            std::dynamic_pointer_cast<codegen::LabelOperand>(operand)) {
      return std::make_shared<codegen::LabelOperand>(*label_ptr);
    }
    throw std::runtime_error(
        "Unsupported operand type during register allocation");
  };

  const auto translated_operand1 = translate_operand(node.operand1);
  const auto translated_operand2 = translate_operand(node.operand2);
  if (translated_operand2 && !translated_operand1) {
    throw std::runtime_error("Invalid operation with operand2 but no operand1");
  }

  if (node.opcode == codegen::OpCode::MOV && translated_operand1 &&
      translated_operand2 &&
      same_operand(translated_operand1, translated_operand2)) {
    // move coalescing, do not unneeded add mov instruction
  } else if (translated_operand1 && translated_operand2) {
    results.emplace_back(std::make_shared<codegen::Operation>(
        node.opcode, translated_operand1, translated_operand2));
  } else if (translated_operand1) {
    results.emplace_back(
        std::make_shared<codegen::Operation>(node.opcode, translated_operand1));
  } else {
    results.emplace_back(std::make_shared<codegen::Operation>(node.opcode));
  }

  // shuttle from registers to stack
  for (const auto& temp : spilled_operands) {
    if (!defined_temps.contains(temp.name)) {
      continue;
    }
    if (!shuttle_indices.contains(temp.name)) {
      shuttle_indices[temp.name] = shuttle_indices.size() + 1;
    }
    results.emplace_back(std::make_shared<codegen::Operation>(
        codegen::OpCode::MOV,
        std::make_shared<codegen::MemoryOperand>(
            codegen::Size::QWORD_PTR,
            codegen::RegisterOperand(codegen::Register::RBP), std::nullopt,
            std::nullopt,
            codegen::ConstantOperand(
                -8 * spilled_indices.at(stack_slot_key(temp.name)))),
        std::make_shared<codegen::RegisterOperand>(
            alloc_reg(shuttle_indices[temp.name]))));
  }

  return codegen::CommandSequence(results);
}

}  // namespace ssy38::my_compiler::asm_cfg
