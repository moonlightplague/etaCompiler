#include "cfg.hpp"

#include <algorithm>
#include <deque>
#include <format>
#include <stack>
#include <unordered_map>

namespace ssy38::my_compiler::cfg {
std::unordered_map<std::string, std::shared_ptr<node>> LABEL2NODE;
std::unordered_set<copy> COPIES;
std::unordered_set<std::string> TEMPS;
std::size_t ARG_COUNT;

namespace {
constexpr int64_t kUnknownConst = std::numeric_limits<int64_t>::min();

void meet_const_map(const const_map &input, const_map &result) {
  for (const auto &[name, value] : input) {
    auto [it, inserted] = result.try_emplace(name, value);
    if (!inserted && it->second != kUnknownConst && it->second != value) {
      it->second = kUnknownConst;
    }
  }
}

bool ends_basic_block(stmt_type type) {
  return type == stmt_type::Cjump || type == stmt_type::Jump ||
         type == stmt_type::Return;
}
}  // namespace

std::unordered_set<copy> copy_intersect(const std::unordered_set<copy> &A,
                                        const std::unordered_set<copy> &B) {
  auto result = std::unordered_set<copy>({});
  const auto &smaller = A.size() < B.size() ? A : B;
  const auto &larger = A.size() < B.size() ? B : A;
  result.reserve(smaller.size());
  for (const auto &cp : smaller) {
    if (larger.contains(cp)) result.insert(cp);
  }
  return result;
}
std::unordered_set<copy> copy_union(const std::unordered_set<copy> &A,
                                    const std::unordered_set<copy> &B) {
  auto result = std::unordered_set<copy>({});
  result.reserve(A.size() + B.size());
  for (const auto &cp : A) result.insert(cp);
  for (const auto &cp : B) result.insert(cp);
  return result;
}
std::unordered_set<copy> copy_minus(const std::unordered_set<copy> &A,
                                    const std::unordered_set<copy> &B) {
  auto result = std::unordered_set<copy>({});
  result.reserve(A.size());
  for (const auto &cp : A)
    if (!B.contains(cp)) result.insert(cp);
  return result;
}
std::string canon(const std::unordered_set<copy> &A, const std::string t) {
  auto current = t;
  auto visited = std::unordered_set<std::string>({});
  while (visited.insert(current).second) {
    auto next = std::find_if(A.begin(), A.end(), [&current](const copy &cp) {
      return cp.first == current;
    });
    if (next == A.end()) return current;
    current = next->second;
  }
  return current;
}

std::unordered_set<std::string> node::use() {
  switch (type) {
    case stmt_type::Assign: {
      auto s = std::dynamic_pointer_cast<ir::Assignment>(stmt);
      if (!s)
        throw std::runtime_error(
            "An Assign node doesn't have a Assignment statement");
      return s->expression->temps_of();
    }
    case stmt_type::MemAssign: {
      auto s = std::dynamic_pointer_cast<ir::Assignment>(stmt);
      if (!s)
        throw std::runtime_error(
            "A MemAssign node doesn't have a Assignment statement");
      auto right_set = s->expression->temps_of();
      if (!std::holds_alternative<ir::MemoryAccess>(s->dest)) {
        throw std::runtime_error(
            "A MemAssign node doesn't have a MemoryAccess destination");
      }
      auto m = std::get<ir::MemoryAccess>(s->dest);
      auto left_set = m.temps_of();
      right_set.insert(left_set.begin(), left_set.end());
      return right_set;
    }
    case stmt_type::Call: {
      auto s = std::dynamic_pointer_cast<ir::CallStatement>(stmt);
      if (!s)
        throw std::runtime_error("A Call node doesn't have a CallStatement");
      std::unordered_set<std::string> use;
      for (auto e : s->arguments) {
        auto temps = e->temps_of();
        use.insert(temps.begin(), temps.end());
      }
      return use;
    }
    case stmt_type::Jump:
      return std::unordered_set<std::string>();
    case stmt_type::Cjump: {
      auto s = std::dynamic_pointer_cast<ir::ConditionalJump>(stmt);
      if (!s)
        throw std::runtime_error(
            "A Cjump node doesn't have a ConditionalJump statement");
      return s->condition->temps_of();
    }
    case stmt_type::Return: {
      auto s = std::dynamic_pointer_cast<ir::ReturnStatement>(stmt);
      if (!s)
        throw std::runtime_error(
            "A Return node doesn't have a ReturnStatement");
      std::unordered_set<std::string> use;
      for (auto e : s->results) {
        auto temps = e->temps_of();
        use.insert(temps.begin(), temps.end());
      }
      return use;
    }
    case stmt_type::Label:
      return std::unordered_set<std::string>();
    default:
      return std::unordered_set<std::string>();
  }
}
std::unordered_set<std::string> node::def() {
  switch (type) {
    case stmt_type::Assign: {
      auto s = std::dynamic_pointer_cast<ir::Assignment>(stmt);
      if (!s)
        throw std::runtime_error(
            "An Assign node doesn't have a Assignment statement");
      if (!std::holds_alternative<ir::Temporary>(s->dest)) {
        throw std::runtime_error(
            "An Assign node doesn't have a Temporary destination");
      }
      auto m = std::get<ir::Temporary>(s->dest);
      return std::unordered_set<std::string>{m.name};
    }
    case stmt_type::Call: {
      auto s = std::dynamic_pointer_cast<ir::CallStatement>(stmt);
      if (!s)
        throw std::runtime_error(
            "An Call node doesn't have a CallStatement statement");
      std::unordered_set<std::string> def;
      for (std::size_t i = 1; i <= s->numReturns; ++i) {
        def.insert(std::format("_RV{}", i));
      }
      return def;
    }
    default:
      return std::unordered_set<std::string>();
  }
}
std::unordered_set<copy> node::gen() {
  if (type == stmt_type::Assign) {
    auto s = std::dynamic_pointer_cast<ir::Assignment>(stmt);
    if (!s)
      throw std::runtime_error(
          "An Assign node doesn't have a Assignment statement");
    auto right = std::dynamic_pointer_cast<ir::Temporary>(s->expression);
    if (!right) return std::unordered_set<copy>({});
    if (!std::holds_alternative<ir::Temporary>(s->dest)) {
      throw std::runtime_error(
          "An Assign node doesn't have a Temporary destination");
    }
    auto left = std::get<ir::Temporary>(s->dest);
    if (left.name != right->name) {
      return std::unordered_set<copy>{copy(left.name, right->name)};
    }
  }
  return std::unordered_set<copy>({});
}
std::unordered_set<copy> node::kill() {
  auto defs = def();
  auto result = std::unordered_set<copy>({});
  if (!defs.empty()) {
    for (const auto &cp : COPIES)
      if (defs.contains(cp.first) || defs.contains(cp.second))
        result.insert(cp);
  }
  return result;
}
void node::rewrite(const std::unordered_set<copy> &A) {
  switch (type) {
    case stmt_type::Assign: {
      auto s = std::dynamic_pointer_cast<ir::Assignment>(stmt);
      if (!s)
        throw std::runtime_error(
            "An Assign node doesn't have a Assignment statement");
      stmt = std::make_shared<ir::Assignment>(
          s->dest, std::any_cast<std::shared_ptr<ir::Expression>>(
                       s->expression->rewrite(A)));
      break;
    }
    case stmt_type::MemAssign: {
      auto s = std::dynamic_pointer_cast<ir::Assignment>(stmt);
      if (!s)
        throw std::runtime_error(
            "An MemAssign node doesn't have a Assignment statement");
      if (!std::holds_alternative<ir::MemoryAccess>(s->dest)) {
        throw std::runtime_error(
            "A MemAssign node doesn't have a MemoryAccess destination");
      }
      auto m = std::get<ir::MemoryAccess>(s->dest);
      auto m_reconstructed = ir::MemoryAccess(
          std::any_cast<std::shared_ptr<ir::Expression>>(m.expr->rewrite(A)));
      stmt = std::make_shared<ir::Assignment>(
          ir::Destination(m_reconstructed),
          std::any_cast<std::shared_ptr<ir::Expression>>(
              s->expression->rewrite(A)));
      break;
    }
    case stmt_type::Call: {
      auto s = std::dynamic_pointer_cast<ir::CallStatement>(stmt);
      if (!s)
        throw std::runtime_error("A Call node doesn't have a CallStatement");
      std::vector<std::shared_ptr<ir::Expression>> new_arguments;
      new_arguments.reserve(s->arguments.size());
      for (auto e : s->arguments) {
        auto new_e =
            std::any_cast<std::shared_ptr<ir::Expression>>(e->rewrite(A));
        new_arguments.push_back(new_e);
      }
      stmt = std::make_shared<ir::CallStatement>(s->function, new_arguments,
                                                 s->numReturns);
      break;
    }
    case stmt_type::Jump: {
      return;
    }
    case stmt_type::Cjump: {
      auto s = std::dynamic_pointer_cast<ir::ConditionalJump>(stmt);
      if (!s)
        throw std::runtime_error(
            "A Cjump node doesn't have a ConditionalJump statement");
      stmt = std::make_shared<ir::ConditionalJump>(
          std::any_cast<std::shared_ptr<ir::Expression>>(
              s->condition->rewrite(A)),
          s->ifLabelName, s->elseLabelName);
      break;
    }
    case stmt_type::Return: {
      auto s = std::dynamic_pointer_cast<ir::ReturnStatement>(stmt);
      if (!s)
        throw std::runtime_error(
            "A Return node doesn't have a ReturnStatement");
      std::vector<std::shared_ptr<ir::Expression>> new_results;
      for (auto e : s->results) {
        auto new_e =
            std::any_cast<std::shared_ptr<ir::Expression>>(e->rewrite(A));
        new_results.push_back(new_e);
      }
      stmt = std::make_shared<ir::ReturnStatement>(new_results);
      break;
    }
    case stmt_type::Label: {
      return;
    }
    default:
      return;
  }
}
const_map node::ccp_transfer(const const_map &input) {
  switch (type) {
    case (stmt_type::Assign): {
      const_map output = input;
      auto s = std::dynamic_pointer_cast<ir::Assignment>(stmt);
      if (!s)
        throw std::runtime_error(
            "An Assign node doesn't have a Assignment statement");
      if (!std::holds_alternative<ir::Temporary>(s->dest)) {
        throw std::runtime_error(
            "An Assign node doesn't have a Temporary destination");
      }
      auto m = std::get<ir::Temporary>(s->dest);
      output[m.name] = s->expression->CCP_eval(input);
      return output;
    }
    case (stmt_type::Call): {
      const_map output = input;
      auto s = std::dynamic_pointer_cast<ir::CallStatement>(stmt);
      if (!s)
        throw std::runtime_error("A Call node doesn't have a CallStatement");
      for (std::size_t i = 1; i <= s->numReturns; ++i) {
        output[std::format("_RV{}", i)] = std::numeric_limits<int64_t>::min();
      }
      return output;
    }
    default:
      return input;
  }
}
bool node::unreachable_transfer(std::shared_ptr<node> n) {
  switch (type) {
    case (stmt_type::Cjump): {
      auto s = std::dynamic_pointer_cast<ir::ConditionalJump>(stmt);
      if (!s)
        throw std::runtime_error(
            "A Cjump node doesn't have a ConditionalJump statement");
      auto v = s->condition->CCP_eval(in_ccp);
      if (n == next) {
        if (v != 0 && v != std::numeric_limits<int64_t>::min())
          return true;
        else
          return unreachable;
      }
      if (n == ifNode) {
        if (v == 0)
          return true;
        else
          return unreachable;
      }
      throw std::runtime_error("n is not a successor node of the current node");
    }
    default:
      return unreachable;
  }
}
void node::const_rewrite(const const_map &A) {
  switch (type) {
    case stmt_type::Assign: {
      auto s = std::dynamic_pointer_cast<ir::Assignment>(stmt);
      if (!s)
        throw std::runtime_error(
            "An Assign node doesn't have a Assignment statement");
      stmt = std::make_shared<ir::Assignment>(
          s->dest, std::any_cast<std::shared_ptr<ir::Expression>>(
                       s->expression->const_rewrite(A)));
      break;
    }
    case stmt_type::MemAssign: {
      auto s = std::dynamic_pointer_cast<ir::Assignment>(stmt);
      if (!s)
        throw std::runtime_error(
            "An MemAssign node doesn't have a Assignment statement");
      if (!std::holds_alternative<ir::MemoryAccess>(s->dest)) {
        throw std::runtime_error(
            "A MemAssign node doesn't have a MemoryAccess destination");
      }
      auto m = std::get<ir::MemoryAccess>(s->dest);
      auto m_reconstructed =
          ir::MemoryAccess(std::any_cast<std::shared_ptr<ir::Expression>>(
              m.expr->const_rewrite(A)));
      stmt = std::make_shared<ir::Assignment>(
          ir::Destination(m_reconstructed),
          std::any_cast<std::shared_ptr<ir::Expression>>(
              s->expression->const_rewrite(A)));
      break;
    }
    case stmt_type::Call: {
      auto s = std::dynamic_pointer_cast<ir::CallStatement>(stmt);
      if (!s)
        throw std::runtime_error("A Call node doesn't have a CallStatement");
      std::vector<std::shared_ptr<ir::Expression>> new_arguments;
      new_arguments.reserve(s->arguments.size());
      for (auto e : s->arguments) {
        auto new_e =
            std::any_cast<std::shared_ptr<ir::Expression>>(e->const_rewrite(A));
        new_arguments.push_back(new_e);
      }
      stmt = std::make_shared<ir::CallStatement>(s->function, new_arguments,
                                                 s->numReturns);
      break;
    }
    case stmt_type::Jump: {
      return;
    }
    case stmt_type::Cjump: {
      auto s = std::dynamic_pointer_cast<ir::ConditionalJump>(stmt);
      if (!s)
        throw std::runtime_error(
            "A Cjump node doesn't have a ConditionalJump statement");
      stmt = std::make_shared<ir::ConditionalJump>(
          std::any_cast<std::shared_ptr<ir::Expression>>(
              s->condition->const_rewrite(A)),
          s->ifLabelName, s->elseLabelName);
      break;
    }
    case stmt_type::Return: {
      auto s = std::dynamic_pointer_cast<ir::ReturnStatement>(stmt);
      if (!s)
        throw std::runtime_error(
            "A Return node doesn't have a ReturnStatement");
      std::vector<std::shared_ptr<ir::Expression>> new_results;
      for (auto e : s->results) {
        auto new_e =
            std::any_cast<std::shared_ptr<ir::Expression>>(e->const_rewrite(A));
        new_results.push_back(new_e);
      }
      stmt = std::make_shared<ir::ReturnStatement>(new_results);
      break;
    }
    case stmt_type::Label: {
      return;
    }
    default:
      return;
  }
}
ir::vn::vnTable node::vn_transfer(ir::vn::vnTable new_vn) {
  switch (type) {
    case (stmt_type::Assign): {
      auto s = std::dynamic_pointer_cast<ir::Assignment>(stmt);
      if (!s)
        throw std::runtime_error(
            "An Assign node doesn't have a Assignment statement");
      if (!std::holds_alternative<ir::Temporary>(s->dest)) {
        throw std::runtime_error(
            "An Assign node doesn't have a Temporary destination");
      }
      auto m = std::get<ir::Temporary>(s->dest);
      auto [key, any_expr] = s->expression->vn_key_rewrite(new_vn);
      stmt = std::make_shared<ir::Assignment>(
          s->dest, std::any_cast<std::shared_ptr<ir::Expression>>(any_expr));
      if (new_vn.tempVN.contains(m.name)) {  // remove original key
        auto old_key = new_vn.tempVN[m.name];
        new_vn.vnTemps[old_key].erase(m.name);
      }
      new_vn.tempVN[m.name] = key;
      if (new_vn.vnTemps.contains(key))
        new_vn.vnTemps[key].insert(m.name);
      else
        new_vn.vnTemps[key] = {m.name};
      return new_vn;
    }
    case (stmt_type::MemAssign): {
      auto s = std::dynamic_pointer_cast<ir::Assignment>(stmt);
      if (!s)
        throw std::runtime_error(
            "An MemAssign node doesn't have a Assignment statement");
      if (!std::holds_alternative<ir::MemoryAccess>(s->dest)) {
        throw std::runtime_error(
            "A MemAssign node doesn't have a MemoryAccess destination");
      }
      auto m = std::get<ir::MemoryAccess>(s->dest);
      auto any_expr_right = std::get<1>(s->expression->vn_key_rewrite(new_vn));
      auto any_expr_left = std::get<1>(m.expr->vn_key_rewrite(new_vn));
      stmt = std::make_shared<ir::Assignment>(
          ir::Destination(ir::MemoryAccess(
              std::any_cast<std::shared_ptr<ir::Expression>>(any_expr_left))),
          std::any_cast<std::shared_ptr<ir::Expression>>(any_expr_right));
      return new_vn;
    }
    case (stmt_type::Call): {
      auto s = std::dynamic_pointer_cast<ir::CallStatement>(stmt);
      if (!s)
        throw std::runtime_error("A Call node doesn't have a CallStatement");
      std::vector<std::shared_ptr<ir::Expression>> new_arg_list;
      new_arg_list.reserve(s->arguments.size());
      for (auto a : s->arguments) {
        auto any_expr = std::get<1>(a->vn_key_rewrite(new_vn));
        new_arg_list.push_back(
            std::any_cast<std::shared_ptr<ir::Expression>>(any_expr));
      }
      stmt = std::make_shared<ir::CallStatement>(s->function, new_arg_list,
                                                 s->numReturns);
      for (size_t i = 1; i <= s->numReturns;
           i++) {  // assign fresh VN to all _RV{} after call
        auto name = std::format("_RV{}", i);
        if (new_vn.tempVN.contains(name)) {  // remove old _RVs
          auto old_key = new_vn.tempVN[name];
          new_vn.vnTemps[old_key].erase(name);
        }
        new_vn.tempVN[name] = new_vn.current_value;
        new_vn.vnTemps[new_vn.current_value] = {name};
        new_vn.current_value++;
      }
      return new_vn;
    }
    case (stmt_type::Cjump): {
      auto s = std::dynamic_pointer_cast<ir::ConditionalJump>(stmt);
      if (!s)
        throw std::runtime_error(
            "A Cjump node doesn't have a ConditionalJump statement");
      auto any_expr = std::get<1>(s->condition->vn_key_rewrite(new_vn));
      stmt = std::make_shared<ir::ConditionalJump>(
          std::any_cast<std::shared_ptr<ir::Expression>>(any_expr),
          s->ifLabelName, s->elseLabelName);
      return new_vn;
    }
    case (stmt_type::Return): {
      auto s = std::dynamic_pointer_cast<ir::ReturnStatement>(stmt);
      if (!s)
        throw std::runtime_error(
            "A Return node doesn't have a ReturnStatement");
      std::vector<std::shared_ptr<ir::Expression>> new_arg_list;
      new_arg_list.reserve(s->results.size());
      for (auto a : s->results) {
        auto any_expr = std::get<1>(a->vn_key_rewrite(new_vn));
        new_arg_list.push_back(
            std::any_cast<std::shared_ptr<ir::Expression>>(any_expr));
      }
      stmt = std::make_shared<ir::ReturnStatement>(new_arg_list);
      return new_vn;
    }
    default:
      return new_vn;
  }
}
void graph::reconnect() {
  LABEL2NODE.clear();
  for (auto &n : node_list) {
    n->pred.clear();
    n->next = nullptr;
    n->ifNode = nullptr;
    if (n->type == stmt_type::Label) {
      auto s = std::dynamic_pointer_cast<ir::Label>(n->stmt);
      if (!s)
        throw std::runtime_error("A Label node doesn't have a Label statement");
      if (!cfg::LABEL2NODE.contains(s->name))
        cfg::LABEL2NODE[s->name] = n;
      else
        throw std::runtime_error(
            std::format("Duplicated label name {} detected.", s->name));
    }
  }
  for (std::size_t i = 0; i < node_list.size(); ++i) {
    auto n = node_list[i];
    if (n->type == stmt_type::Jump) {
      auto stmt = std::dynamic_pointer_cast<ir::Jump>(n->stmt);
      if (!stmt)
        throw std::runtime_error("A Jump node doesn't have a Jump statement");
      auto address = std::dynamic_pointer_cast<ir::Address>(stmt->target);
      if (!address)
        throw std::runtime_error("A Jump statement's target is not an Address");
      n->next = LABEL2NODE.at(address->label);
      LABEL2NODE.at(address->label)->pred.push_back(n);
      continue;
    }
    if (n->type == stmt_type::Cjump) {
      auto stmt = std::dynamic_pointer_cast<ir::ConditionalJump>(n->stmt);
      if (!stmt)
        throw std::runtime_error(
            "A Cjump node doesn't have a ConditionalJump statement");
      if (i < node_list.size() - 1) {
        n->next = node_list[i + 1];
        n->ifNode = LABEL2NODE.at(stmt->ifLabelName);
        LABEL2NODE.at(stmt->ifLabelName)->pred.push_back(n);
        node_list[i + 1]->pred.push_back(n);
      } else {
        n->ifNode = LABEL2NODE.at(stmt->ifLabelName);
        LABEL2NODE.at(stmt->ifLabelName)->pred.push_back(n);
      }
      continue;
    }
    if (n->type == stmt_type::Return || i == node_list.size() - 1) continue;
    n->next = node_list[i + 1];
    node_list[i + 1]->pred.push_back(n);
  }
}
bool graph::unreachable_elim() {
  if (node_list.empty()) {
    return false;
  }

  std::unordered_map<node *, std::size_t> node_to_index;
  node_to_index.reserve(node_list.size());
  for (std::size_t i = 0; i < node_list.size(); ++i) {
    node_to_index[node_list[i].get()] = i;
  }

  std::vector<bool> reachable(node_list.size(), false);
  std::stack<std::shared_ptr<node>> worklist;
  worklist.push(node_list.front());

  while (!worklist.empty()) {
    std::shared_ptr<node> current = worklist.top();
    worklist.pop();
    if (current == nullptr) {
      continue;
    }

    auto it = node_to_index.find(current.get());
    if (it == node_to_index.end()) {
      continue;
    }

    std::size_t idx = it->second;
    if (reachable[idx]) {
      continue;
    }
    reachable[idx] = true;

    if (current->next != nullptr) {
      worklist.push(current->next);
    }
    if (current->ifNode != nullptr) {
      worklist.push(current->ifNode);
    }
  }

  std::vector<std::shared_ptr<node>> reachable_nodes;
  reachable_nodes.reserve(node_list.size());
  for (std::size_t i = 0; i < node_list.size(); ++i) {
    if (reachable[i]) {
      reachable_nodes.push_back(node_list[i]);
    }
  }
  bool result = node_list.size() > reachable_nodes.size();
  node_list = reachable_nodes;
  return result;
}
bool graph::dead_code_elim() {
  for (auto &n : node_list) {
    n->in_dce = std::unordered_set<std::string>({});
    n->out_dce = std::unordered_set<std::string>({});
  }
  std::deque<std::shared_ptr<node>> worklist;
  std::unordered_set<node *> queued;

  // construct the worklist
  for (const auto &item : node_list) {
    worklist.push_back(item);
    queued.insert(item.get());
  }

  // start dataflow analysis
  while (!worklist.empty()) {
    auto n = worklist.front();
    worklist.pop_front();
    queued.erase(n.get());
    std::unordered_set<std::string> new_in;
    std::unordered_set<std::string> new_out;
    if (n->next) new_out.insert(n->next->in_dce.begin(), n->next->in_dce.end());
    if (n->ifNode)
      new_out.insert(n->ifNode->in_dce.begin(), n->ifNode->in_dce.end());
    new_in = new_out;
    auto defs = n->def();
    for (const auto &t : defs) {
      new_in.erase(t);
    }
    auto uses = n->use();
    new_in.insert(uses.begin(), uses.end());
    if (new_in != n->in_dce || new_out != n->out_dce) {
      n->in_dce = new_in;
      n->out_dce = new_out;
      for (auto prev : n->pred) {
        if (queued.insert(prev.get()).second) {
          worklist.push_back(prev);
        }
      }
    }
  }

  // remove dead nodes
  std::vector<bool> live_mark(node_list.size(), true);
  for (std::size_t i = 1; i < node_list.size(); ++i) {
    auto &n = node_list[i];
    if (n->type == stmt_type::Assign) {
      auto s = std::dynamic_pointer_cast<ir::Assignment>(n->stmt);
      if (!s)
        throw std::runtime_error(
            "An Assign node doesn't have a Assignment statement");
      if (!std::holds_alternative<ir::Temporary>(s->dest)) {
        throw std::runtime_error(
            "An Assign node doesn't have a Temporary destination");
      }
      auto m = std::get<ir::Temporary>(s->dest);
      if (n->out_dce.find(m.name) == n->out_dce.end()) {
        live_mark[i] = false;
      }
    }
  }
  std::vector<std::shared_ptr<node>> new_nodes;
  new_nodes.reserve(node_list.size());
  for (std::size_t i = 0; i < node_list.size(); ++i) {
    if (live_mark[i]) new_nodes.push_back(node_list[i]);
  }
  bool result = node_list.size() > new_nodes.size();
  node_list = new_nodes;
  return result;
}
bool graph::copy_propag() {
  if (node_list.empty()) {
    return false;
  }
  COPIES.clear();
  for (const auto &n : node_list) {
    if (!n) continue;
    auto generated = n->gen();
    COPIES.insert(generated.begin(), generated.end());
  }
  // initialize the in and out of each node
  node_list[0]->in_cp = std::unordered_set<copy>({});
  node_list[0]->out_cp = std::unordered_set<copy>({});
  for (std::size_t i = 1; i < node_list.size(); ++i) {
    node_list[i]->in_cp = std::unordered_set<copy>(COPIES);
    node_list[i]->out_cp = std::unordered_set<copy>(COPIES);
  }

  // initialize the worklist
  std::deque<std::shared_ptr<node>> worklist;
  std::unordered_set<node *> queued;
  for (std::size_t i = 1; i < node_list.size(); ++i) {
    worklist.push_back(node_list[i]);
    queued.insert(node_list[i].get());
  }

  // start dataflow analysis
  while (!worklist.empty()) {
    auto n = worklist.front();
    worklist.pop_front();
    queued.erase(n.get());
    if (!n) continue;
    std::unordered_set<copy> new_in;
    if (!n->pred.empty()) {
      new_in = COPIES;
      for (auto prev : n->pred) {
        new_in = copy_intersect(new_in, prev->out_cp);
      }
    }
    auto gen = n->gen();
    auto kill = n->kill();
    std::unordered_set<copy> new_out =
        copy_union(gen, copy_minus(new_in, kill));
    if (new_in != n->in_cp || new_out != n->out_cp) {
      n->in_cp = new_in;
      n->out_cp = new_out;
      if (n->next && queued.insert(n->next.get()).second)
        worklist.push_back(n->next);
      if (n->ifNode && queued.insert(n->ifNode.get()).second)
        worklist.push_back(n->ifNode);
    }
  }

  // rewrite nodes
  std::vector<bool> live_mark(node_list.size(), true);
  for (std::size_t i = 1; i < node_list.size(); ++i) {
    auto &n = node_list[i];
    n->rewrite(n->in_cp);
    if (n->type == stmt_type::Assign) {
      auto s = std::dynamic_pointer_cast<ir::Assignment>(n->stmt);
      if (!s)
        throw std::runtime_error(
            "An Assign node doesn't have a Assignment statement");
      if (!std::holds_alternative<ir::Temporary>(s->dest)) {
        throw std::runtime_error(
            "An Assign node doesn't have a Temporary destination");
      }
      auto m = std::get<ir::Temporary>(s->dest);
      auto right = std::dynamic_pointer_cast<ir::Temporary>(s->expression);
      if (right && m.name == right->name) {
        live_mark[i] = false;
      }
    }
  }
  std::vector<std::shared_ptr<node>> new_nodes;
  new_nodes.reserve(node_list.size());
  for (std::size_t i = 0; i < node_list.size(); ++i) {
    if (live_mark[i]) new_nodes.push_back(node_list[i]);
  }
  bool result = node_list.size() > new_nodes.size();
  node_list = new_nodes;
  return result;
}
void graph::const_propag() {
  if (node_list.empty()) {
    return;
  }
  for (std::size_t i = 0; i < node_list.size(); ++i) {
    auto n = node_list[i];
    n->in_ccp = const_map();
    n->out_ccp = const_map();
    n->unreachable = true;
  }
  auto start = node_list[0];
  for (std::size_t i = 1; i <= ARG_COUNT; i++) {
    start->unreachable = false;
    start->in_ccp[std::format("_ARG{}", i)] =
        std::numeric_limits<int64_t>::min();
    start->out_ccp[std::format("_ARG{}", i)] =
        std::numeric_limits<int64_t>::min();
  }
  start->next->unreachable = false;

  // construct worklist
  std::deque<std::shared_ptr<node>> worklist;
  std::unordered_set<node *> queued;
  for (std::size_t i = 1; i < node_list.size(); ++i) {
    worklist.push_back(node_list[i]);
    queued.insert(node_list[i].get());
  }

  // start dataflow analysis
  while (!worklist.empty()) {
    auto n = worklist.front();
    worklist.pop_front();
    queued.erase(n.get());

    // calculate unreachable for the current node
    for (auto prev : n->pred) {
      if (!prev->unreachable)
        n->unreachable = n->unreachable && prev->unreachable_transfer(n);
    }
    if (n->unreachable) continue;

    // calculate meet
    const_map new_in;
    for (const auto &prev : n->pred) {
      if (!prev->unreachable) {
        meet_const_map(prev->out_ccp, new_in);
      }
    }

    // calculate transfer
    auto new_out = n->ccp_transfer(new_in);
    if (new_in != n->in_ccp || new_out != n->out_ccp) {
      n->in_ccp = new_in;
      n->out_ccp = new_out;
      if (n->next && queued.insert(n->next.get()).second)
        worklist.push_back(n->next);
      if (n->ifNode && queued.insert(n->ifNode.get()).second)
        worklist.push_back(n->ifNode);
    }
  }
  // rewrite nodes
  for (std::size_t i = 1; i < node_list.size(); ++i) {
    auto &n = node_list[i];
    if (!n->unreachable) n->const_rewrite(n->in_ccp);
  }
}
void graph::control_flow_elim() {
  std::vector<std::shared_ptr<node>> new_nodes;
  bool loop = true;
  while (loop) {
    loop = false;
    new_nodes.clear();
    new_nodes.reserve(node_list.size());
    std::vector<bool> live_mark(node_list.size(), true);
    for (size_t i = 1; i < node_list.size(); ++i) {
      auto n = node_list[i];
      if (n->type == stmt_type::Cjump) {
        auto s = std::dynamic_pointer_cast<ir::ConditionalJump>(n->stmt);
        if (!s)
          throw std::runtime_error(
              "A Cjump node doesn't have a ConditionalJump statement");
        if (LABEL2NODE[s->ifLabelName] == node_list[i + 1]) {
          live_mark[i] = false;
          loop = true;
          continue;
        }
        auto is_const =
            std::dynamic_pointer_cast<ir::IntegerLiteral>(s->condition);
        if (!is_const) continue;
        if (is_const->value == std::numeric_limits<int64_t>::min())
          throw std::runtime_error("NAC not supported");
        if (is_const->value == 0) {
          live_mark[i] = false;
          loop = true;
          continue;
        }
        loop = true;
        n->type = stmt_type::Jump;
        n->stmt =
            std::static_pointer_cast<ir::Statement>(std::make_shared<ir::Jump>(
                std::make_shared<ir::Address>(s->ifLabelName)));
        continue;
      }
      if (n->type == stmt_type::Jump) {
        auto s = std::dynamic_pointer_cast<ir::Jump>(n->stmt);
        if (!s)
          throw std::runtime_error("A Jump node doesn't have a Jump statement");
        auto address = std::dynamic_pointer_cast<ir::Address>(s->target);
        if (!address)
          throw std::runtime_error(
              "A Jump statement's target is not an Address");
        if (LABEL2NODE[address->label] == node_list[i + 1]) {
          live_mark[i] = false;
          loop = true;
        }
      }
      if (n->type == stmt_type::Label) {
        if (n->pred.size() == 0) {
          live_mark[i] = false;
          loop = true;
          continue;
        }
        if (n->pred.size() == 1 && n->pred[0] == node_list[i - 1]) {
          live_mark[i] = false;
          loop = true;
        }
      }
    }
    for (size_t i = 0; i < node_list.size(); ++i) {
      if (live_mark[i]) new_nodes.push_back(node_list[i]);
    }
    node_list = new_nodes;
    if (loop) reconnect();
  }
}
void graph::value_numbering() {
  ir::vn::vnTable current_vn;
  bool block_start = true;
  for (auto &n : node_list) {
    if (block_start || n->type == stmt_type::Label) {
      current_vn = ir::vn::vnTable();
    }
    current_vn = n->vn_transfer(std::move(current_vn));
    block_start = ends_basic_block(n->type);
  }
}
}  // namespace ssy38::my_compiler::cfg
