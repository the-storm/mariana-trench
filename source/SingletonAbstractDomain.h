/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <AbstractDomain.h>

#include <mariana-trench/Assert.h>
#include <mariana-trench/IncludeMacros.h>

namespace marianatrench {

class SingletonAbstractValue final
    : public sparta::AbstractValue<SingletonAbstractValue> {
 private:
  using AbstractValueKind = sparta::AbstractValueKind;

 public:
  SingletonAbstractValue() = default;

  INCLUDE_DEFAULT_COPY_CONSTRUCTORS_AND_ASSIGNMENTS(SingletonAbstractValue)

  void clear() {}

  AbstractValueKind kind() const {
    return AbstractValueKind::Value;
  }

  bool leq(const SingletonAbstractValue& other) const {
    return equals(other);
  }

  bool equals(const SingletonAbstractValue& /* unused */) const {
    return true;
  }

  AbstractValueKind join_with(const SingletonAbstractValue& /* unused */) {
    return AbstractValueKind::Value;
  }

  AbstractValueKind widen_with(const SingletonAbstractValue& other) {
    return join_with(other);
  }

  AbstractValueKind meet_with(const SingletonAbstractValue& /* unused */) {
    mt_unreachable();
  }

  AbstractValueKind narrow_with(const SingletonAbstractValue& other) {
    mt_unreachable();
  }
};

/*
 * This is a domain which can have only a single value and otherwise be Top or
 * Bottom. It is used along with the AbstractTreeDomain to store a tree of paths
 * within local taint to keep track of paths for propagations and sinks.
 */
class SingletonAbstractDomain final : public sparta::AbstractDomainScaffolding<
                                          SingletonAbstractValue,
                                          SingletonAbstractDomain> {
 public:
  // Return a `SingletonAbstractDomain` of holding a value which is of kind
  // `AbstractValueKind::Value`.
  SingletonAbstractDomain() = default;

  INCLUDE_DEFAULT_COPY_CONSTRUCTORS_AND_ASSIGNMENTS(SingletonAbstractDomain)

  static SingletonAbstractDomain bottom() {
    auto domain = SingletonAbstractDomain();
    domain.set_to_bottom();
    return domain;
  }

  void difference_with(const SingletonAbstractDomain& other) {
    if (!leq(other)) {
      return;
    }
    set_to_bottom();
  }

  friend std::ostream& operator<<(
      std::ostream& out,
      const SingletonAbstractDomain& domain) {
    switch (domain.kind()) {
      case sparta::AbstractValueKind::Bottom: {
        out << "_|_";
        break;
      }
      case sparta::AbstractValueKind::Top: {
        out << "T";
        break;
      }
      case sparta::AbstractValueKind::Value: {
        out << "Value";
        break;
      }
    }
    return out;
  }
};

} // namespace marianatrench
