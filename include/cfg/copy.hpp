#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <utility>

namespace ssy38::my_compiler::cfg {

// A concrete copy relation from one temporary to another.
struct copy {
  std::string first;
  std::string second;

  copy() = default;
  copy(std::string first, std::string second)
      : first(std::move(first)), second(std::move(second)) {}

  bool operator==(const copy& other) const {
    return first == other.first && second == other.second;
  }
};

using const_map = std::unordered_map<std::string, int64_t>;

}  // namespace ssy38::my_compiler::cfg

template <>
struct std::hash<ssy38::my_compiler::cfg::copy> {
  std::size_t operator()(
      const ssy38::my_compiler::cfg::copy& cp) const noexcept {
    const auto first_hash = std::hash<std::string>{}(cp.first);
    const auto second_hash = std::hash<std::string>{}(cp.second);
    return first_hash ^
           (second_hash + 0x9e3779b9 + (first_hash << 6) + (first_hash >> 2));
  }
};
