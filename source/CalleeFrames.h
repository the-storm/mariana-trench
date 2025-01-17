/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <AbstractDomain.h>
#include <PatriciaTreeMapAbstractPartition.h>

#include <mariana-trench/CallPositionFrames.h>
#include <mariana-trench/IncludeMacros.h>
#include <mariana-trench/KindFactory.h>
#include <mariana-trench/Position.h>
#include <mariana-trench/TaintConfig.h>
#include <mariana-trench/TransformsFactory.h>

namespace marianatrench {

/**
 * Represents a set of frames with the same call position.
 * Based on its position in `Taint`, it is expected that all frames within
 * this class have the same callee and call position.
 */
class CalleeFrames final : public sparta::AbstractDomain<CalleeFrames> {
 private:
  using FramesByCallPosition = sparta::PatriciaTreeMapAbstractPartition<
      const Position * MT_NULLABLE,
      CallPositionFrames>;

 private:
  // Iterator based on `FlattenIterator`.

  struct CallPositionToFramesMapDereference {
    static CallPositionFrames::iterator begin(
        const std::pair<const Position*, CallPositionFrames>& pair) {
      return pair.second.begin();
    }
    static CallPositionFrames::iterator end(
        const std::pair<const Position*, CallPositionFrames>& pair) {
      return pair.second.end();
    }
  };

  using ConstIterator = FlattenIterator<
      /* OuterIterator */ FramesByCallPosition::MapType::iterator,
      /* InnerIterator */ CallPositionFrames::iterator,
      CallPositionToFramesMapDereference>;

 public:
  // C++ container concept member types
  using iterator = ConstIterator;
  using const_iterator = ConstIterator;
  using value_type = Frame;
  using difference_type = std::ptrdiff_t;
  using size_type = std::size_t;
  using const_reference = const Frame&;
  using const_pointer = const Frame*;

 private:
  explicit CalleeFrames(
      const Method* MT_NULLABLE callee,
      CallInfo call_info,
      FramesByCallPosition frames)
      : callee_(callee), call_info_(call_info), frames_(std::move(frames)) {}

 public:
  /* Create the bottom (i.e, empty) frame set. */
  CalleeFrames()
      : callee_(nullptr),
        call_info_(CallInfo::Declaration),
        frames_(FramesByCallPosition::bottom()) {}

  explicit CalleeFrames(std::initializer_list<TaintConfig> configs);

  INCLUDE_DEFAULT_COPY_CONSTRUCTORS_AND_ASSIGNMENTS(CalleeFrames)

  static CalleeFrames bottom() {
    return CalleeFrames(
        /* callee */ nullptr,
        CallInfo::Declaration,
        FramesByCallPosition::bottom());
  }

  static CalleeFrames top() {
    return CalleeFrames(
        /* callee */ nullptr,
        CallInfo::Declaration,
        FramesByCallPosition::top());
  }

  bool is_bottom() const {
    return frames_.is_bottom();
  }

  bool is_top() const {
    return frames_.is_top();
  }

  void set_to_bottom() {
    callee_ = nullptr;
    call_info_ = CallInfo::Declaration;
    frames_.set_to_bottom();
  }

  void set_to_top() {
    callee_ = nullptr;
    call_info_ = CallInfo::Declaration;
    frames_.set_to_top();
  }

  bool empty() const {
    return frames_.is_bottom();
  }

  const Method* MT_NULLABLE callee() const {
    return callee_;
  }

  CallInfo call_info() const {
    return call_info_;
  }

  void add(const TaintConfig& config);

  bool leq(const CalleeFrames& other) const;

  bool equals(const CalleeFrames& other) const;

  void join_with(const CalleeFrames& other);

  void widen_with(const CalleeFrames& other);

  void meet_with(const CalleeFrames& other);

  void narrow_with(const CalleeFrames& other);

  void difference_with(const CalleeFrames& other);

  template <typename Function> // Frame(Frame)
  void map(Function&& f) {
    static_assert(std::is_same_v<decltype(f(std::declval<Frame&&>())), Frame>);

    frames_.map([f = std::forward<Function>(f)](CallPositionFrames frames) {
      frames.map(f);
      return frames;
    });
  }

  template <typename Predicate> // bool(const Frame&)
  void filter(Predicate&& predicate) {
    static_assert(
        std::
            is_same_v<decltype(predicate(std::declval<const Frame&>())), bool>);

    frames_.map([predicate = std::forward<Predicate>(predicate)](
                    CallPositionFrames frames) {
      frames.filter(predicate);
      return frames;
    });
  }

  ConstIterator begin() const {
    return ConstIterator(frames_.bindings().begin(), frames_.bindings().end());
  }

  ConstIterator end() const {
    return ConstIterator(frames_.bindings().end(), frames_.bindings().end());
  }

  void set_origins_if_empty(const MethodSet& origins);

  void set_field_origins_if_empty_with_field_callee(const Field* field);

  FeatureMayAlwaysSet locally_inferred_features(
      const Position* MT_NULLABLE position,
      const AccessPath& callee_port) const;

  void add_locally_inferred_features(const FeatureMayAlwaysSet& features);

  LocalPositionSet local_positions() const;

  void add_local_position(const Position* position);

  void set_local_positions(const LocalPositionSet& positions);

  void add_locally_inferred_features_and_local_position(
      const FeatureMayAlwaysSet& features,
      const Position* MT_NULLABLE position);

  /**
   * Propagate the taint from the callee to the caller.
   *
   * Return bottom if the taint should not be propagated.
   */
  CalleeFrames propagate(
      const Method* callee,
      const AccessPath& callee_port,
      const Position* call_position,
      int maximum_source_sink_distance,
      Context& context,
      const std::vector<const DexType * MT_NULLABLE>& source_register_types,
      const std::vector<std::optional<std::string>>& source_constant_arguments)
      const;

  /* Return the set of leaf frames with the given position. */
  CalleeFrames attach_position(const Position* position) const;

  template <typename TransformKind, typename AddFeatures>
  void transform_kind_with_features(
      TransformKind&& transform_kind, // std::vector<const Kind*>(const Kind*)
      AddFeatures&& add_features // FeatureMayAlwaysSet(const Kind*)
  ) {
    frames_.map([transform_kind = std::forward<TransformKind>(transform_kind),
                 add_features = std::forward<AddFeatures>(add_features)](
                    CallPositionFrames frames) {
      frames.transform_kind_with_features(transform_kind, add_features);
      return frames;
    });
  }

  CalleeFrames apply_transform(
      const KindFactory& kind_factory,
      const TransformsFactory& transforms_factory,
      const UsedKinds& used_kinds,
      const TransformList* local_transforms) const;

  void append_to_propagation_output_paths(Path::Element path_element);

  void update_maximum_collapse_depth(CollapseDepth collapse_depth);

  void update_non_leaf_positions(
      const std::function<
          const Position*(const Method*, const AccessPath&, const Position*)>&
          new_call_position,
      const std::function<LocalPositionSet(const LocalPositionSet&)>&
          new_local_positions);

  void filter_invalid_frames(
      const std::function<
          bool(const Method* MT_NULLABLE, const AccessPath&, const Kind*)>&
          is_valid);

  bool contains_kind(const Kind*) const;

  template <class T>
  std::unordered_map<T, CalleeFrames> partition_by_kind(
      const std::function<T(const Kind*)>& map_kind) const {
    std::unordered_map<T, CalleeFrames> result;
    for (const auto& [position, position_frames] : frames_.bindings()) {
      auto callee_frames_partitioned =
          position_frames.partition_by_kind(map_kind);

      for (const auto& [mapped_value, call_position_frames] :
           callee_frames_partitioned) {
        result[mapped_value].join_with(CalleeFrames(
            callee_,
            call_info_,
            FramesByCallPosition{std::pair(position, call_position_frames)}));
      }
    }
    return result;
  }

  FeatureMayAlwaysSet features_joined() const;

  Json::Value to_json(ExportOriginsMode export_origins_mode) const;

  friend std::ostream& operator<<(
      std::ostream& out,
      const CalleeFrames& frames);

 private:
  const Method* MT_NULLABLE callee_;
  CallInfo call_info_;
  FramesByCallPosition frames_;
};

} // namespace marianatrench
