/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <Show.h>

#include <mariana-trench/MemoryLocationEnvironment.h>

namespace marianatrench {

std::ostream& operator<<(
    std::ostream& out,
    const MemoryLocationsDomain& memory_locations) {
  out << "{";
  for (auto iterator = memory_locations.begin(), end = memory_locations.end();
       iterator != end;) {
    out << "`" << show(*iterator) << "`";
    ++iterator;
    if (iterator != end) {
      out << ", ";
    }
  }
  return out << "}";
}

std::ostream& operator<<(
    std::ostream& out,
    const MemoryLocationEnvironment& memory_locations) {
  if (memory_locations.is_bottom()) {
    return out << "_|_";
  } else if (memory_locations.is_top()) {
    return out << "T";
  } else {
    out << "MemoryLocationEnvironment(";
    for (const auto& entry : memory_locations.bindings()) {
      out << "\n  Register(" << entry.first << ") -> " << entry.second;
    }
    return out << "\n)";
  }
}

} // namespace marianatrench