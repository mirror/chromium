// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/test/animation_timelines_test_common.h"

#include "base/memory/ptr_util.h"
#include "cc/animation/animation_events.h"
#include "cc/animation/animation_id_provider.h"
#include "cc/animation/animation_ticker.h"
#include "cc/animation/animation_timeline.h"
#include "cc/animation/element_animations.h"
#include "cc/animation/single_ticker_animation_player.h"
#include "cc/paint/filter_operation.h"
#include "cc/paint/filter_operations.h"
#include "cc/trees/property_tree.h"
#include "ui/gfx/transform.h"

namespace cc {

std::unique_ptr<TestLayer> TestLayer::Create() {
  return base::WrapUnique(new TestLayer());
}

TestLayer::TestLayer() {
  ClearMutatedProperties();
}

TestLayer::~TestLayer() = default;

void TestLayer::ClearMutatedProperties() {
  transform_ = gfx::Transform();
  opacity_ = 0;
  filters_ = FilterOperations();
  scroll_offset_ = gfx::ScrollOffset();

  has_potential_animation_.reset();
  is_currently_animating_.reset();
  mutated_properties_.reset();
}

int TestLayer::transform_x() const {
  gfx::Vector2dF vec = transform_.To2dTranslation();
  return static_cast<int>(vec.x());
}

int TestLayer::transform_y() const {
  gfx::Vector2dF vec = transform_.To2dTranslation();
  return static_cast<int>(vec.y());
}

float TestLayer::brightness() const {
  for (unsigned i = 0; i < filters_.size(); ++i) {
    const FilterOperation& filter = filters_.at(i);
    if (filter.type() == FilterOperation::BRIGHTNESS)
      return filter.amount();
  }

  NOTREACHED();
  return 0;
}

TestHostClient::TestHostClient(ThreadInstance thread_instance)
    : host_(AnimationHost::CreateForTesting(thread_instance)),
      mutators_need_commit_(false) {
  host_->SetMutatorHostClient(this);
  host_->SetSupportsScrollAnimations(true);
}

TestHostClient::~TestHostClient() {
  host_->SetMutatorHostClient(nullptr);
}

void TestHostClient::ClearMutatedProperties() {
  for (auto& kv : layers_in_pending_tree_)
    kv.second->ClearMutatedProperties();
  for (auto& kv : layers_in_active_tree_)
    kv.second->ClearMutatedProperties();
}

bool TestHostClient::IsElementInList(ElementId element_id,
                                     ElementListType list_type) const {
  return list_type == ElementListType::ACTIVE
             ? layers_in_active_tree_.count(element_id)
             : layers_in_pending_tree_.count(element_id);
}

void TestHostClient::SetMutatorsNeedCommit() {
  mutators_need_commit_ = true;
}

void TestHostClient::SetMutatorsNeedRebuildPropertyTrees() {}

void TestHostClient::SetElementFilterMutated(ElementId element_id,
                                             ElementListType list_type,
                                             const FilterOperations& filters) {
  TestLayer* layer = FindTestLayer(element_id, list_type);
  if (layer)
    layer->set_filters(filters);
}

void TestHostClient::SetElementOpacityMutated(ElementId element_id,
                                              ElementListType list_type,
                                              float opacity) {
  TestLayer* layer = FindTestLayer(element_id, list_type);
  if (layer)
    layer->set_opacity(opacity);
}

void TestHostClient::SetElementTransformMutated(
    ElementId element_id,
    ElementListType list_type,
    const gfx::Transform& transform) {
  TestLayer* layer = FindTestLayer(element_id, list_type);
  if (layer)
    layer->set_transform(transform);
}

void TestHostClient::SetElementScrollOffsetMutated(
    ElementId element_id,
    ElementListType list_type,
    const gfx::ScrollOffset& scroll_offset) {
  TestLayer* layer = FindTestLayer(element_id, list_type);
  if (layer)
    layer->set_scroll_offset(scroll_offset);
}

void TestHostClient::ElementIsAnimatingChanged(
    ElementId element_id,
    ElementListType list_type,
    const PropertyAnimationState& mask,
    const PropertyAnimationState& state) {
  TestLayer* layer = FindTestLayer(element_id, list_type);
  if (!layer)
    return;

  for (int property = TargetProperty::FIRST_TARGET_PROPERTY;
       property <= TargetProperty::LAST_TARGET_PROPERTY; ++property) {
    TargetProperty::Type target_property =
        static_cast<TargetProperty::Type>(property);
    if (mask.potentially_animating[property])
      layer->set_has_potential_animation(target_property,
                                         state.potentially_animating[property]);
    if (mask.currently_running[property])
      layer->set_is_currently_animating(target_property,
                                        state.currently_running[property]);
  }
}

void TestHostClient::SetScrollOffsetForAnimation(
    const gfx::ScrollOffset& scroll_offset) {
  scroll_offset_ = scroll_offset;
}

gfx::ScrollOffset TestHostClient::GetScrollOffsetForAnimation(
    ElementId element_id) const {
  return scroll_offset_;
}

void TestHostClient::RegisterElement(ElementId element_id,
                                     ElementListType list_type) {
  ElementIdToTestLayer& layers_in_tree = list_type == ElementListType::ACTIVE
                                             ? layers_in_active_tree_
                                             : layers_in_pending_tree_;
  DCHECK(layers_in_tree.find(element_id) == layers_in_tree.end());
  layers_in_tree[element_id] = TestLayer::Create();

  DCHECK(host_);
  host_->RegisterElement(element_id, list_type);
}

void TestHostClient::UnregisterElement(ElementId element_id,
                                       ElementListType list_type) {
  DCHECK(host_);
  host_->UnregisterElement(element_id, list_type);

  ElementIdToTestLayer& layers_in_tree = list_type == ElementListType::ACTIVE
                                             ? layers_in_active_tree_
                                             : layers_in_pending_tree_;
  auto kv = layers_in_tree.find(element_id);
  DCHECK(kv != layers_in_tree.end());
  layers_in_tree.erase(kv);
}

bool TestHostClient::IsPropertyMutated(ElementId element_id,
                                       ElementListType list_type,
                                       TargetProperty::Type property) const {
  TestLayer* layer = FindTestLayer(element_id, list_type);
  EXPECT_TRUE(layer);
  return layer->is_property_mutated(property);
}

FilterOperations TestHostClient::GetFilters(ElementId element_id,
                                            ElementListType list_type) const {
  TestLayer* layer = FindTestLayer(element_id, list_type);
  EXPECT_TRUE(layer);
  return layer->filters();
}

float TestHostClient::GetOpacity(ElementId element_id,
                                 ElementListType list_type) const {
  TestLayer* layer = FindTestLayer(element_id, list_type);
  EXPECT_TRUE(layer);
  return layer->opacity();
}

gfx::Transform TestHostClient::GetTransform(ElementId element_id,
                                            ElementListType list_type) const {
  TestLayer* layer = FindTestLayer(element_id, list_type);
  EXPECT_TRUE(layer);
  return layer->transform();
}

gfx::ScrollOffset TestHostClient::GetScrollOffset(
    ElementId element_id,
    ElementListType list_type) const {
  TestLayer* layer = FindTestLayer(element_id, list_type);
  EXPECT_TRUE(layer);
  return layer->scroll_offset();
}

bool TestHostClient::GetTransformIsCurrentlyAnimating(
    ElementId element_id,
    ElementListType list_type) const {
  TestLayer* layer = FindTestLayer(element_id, list_type);
  EXPECT_TRUE(layer);
  return layer->is_currently_animating(TargetProperty::TRANSFORM);
}

bool TestHostClient::GetHasPotentialTransformAnimation(
    ElementId element_id,
    ElementListType list_type) const {
  TestLayer* layer = FindTestLayer(element_id, list_type);
  EXPECT_TRUE(layer);
  return layer->has_potential_animation(TargetProperty::TRANSFORM);
}

bool TestHostClient::GetOpacityIsCurrentlyAnimating(
    ElementId element_id,
    ElementListType list_type) const {
  TestLayer* layer = FindTestLayer(element_id, list_type);
  EXPECT_TRUE(layer);
  return layer->is_currently_animating(TargetProperty::OPACITY);
}

bool TestHostClient::GetHasPotentialOpacityAnimation(
    ElementId element_id,
    ElementListType list_type) const {
  TestLayer* layer = FindTestLayer(element_id, list_type);
  EXPECT_TRUE(layer);
  return layer->has_potential_animation(TargetProperty::OPACITY);
}

bool TestHostClient::GetFilterIsCurrentlyAnimating(
    ElementId element_id,
    ElementListType list_type) const {
  TestLayer* layer = FindTestLayer(element_id, list_type);
  EXPECT_TRUE(layer);
  return layer->is_currently_animating(TargetProperty::FILTER);
}

bool TestHostClient::GetHasPotentialFilterAnimation(
    ElementId element_id,
    ElementListType list_type) const {
  TestLayer* layer = FindTestLayer(element_id, list_type);
  EXPECT_TRUE(layer);
  return layer->has_potential_animation(TargetProperty::FILTER);
}

void TestHostClient::ExpectFilterPropertyMutated(ElementId element_id,
                                                 ElementListType list_type,
                                                 float brightness) const {
  TestLayer* layer = FindTestLayer(element_id, list_type);
  EXPECT_TRUE(layer);
  EXPECT_TRUE(layer->is_property_mutated(TargetProperty::FILTER));
  EXPECT_EQ(brightness, layer->brightness());
}

void TestHostClient::ExpectOpacityPropertyMutated(ElementId element_id,
                                                  ElementListType list_type,
                                                  float opacity) const {
  TestLayer* layer = FindTestLayer(element_id, list_type);
  EXPECT_TRUE(layer);
  EXPECT_TRUE(layer->is_property_mutated(TargetProperty::OPACITY));
  EXPECT_EQ(opacity, layer->opacity());
}

void TestHostClient::ExpectTransformPropertyMutated(ElementId element_id,
                                                    ElementListType list_type,
                                                    int transform_x,
                                                    int transform_y) const {
  TestLayer* layer = FindTestLayer(element_id, list_type);
  EXPECT_TRUE(layer);
  EXPECT_TRUE(layer->is_property_mutated(TargetProperty::TRANSFORM));
  EXPECT_EQ(transform_x, layer->transform_x());
  EXPECT_EQ(transform_y, layer->transform_y());
}

TestLayer* TestHostClient::FindTestLayer(ElementId element_id,
                                         ElementListType list_type) const {
  const ElementIdToTestLayer& layers_in_tree =
      list_type == ElementListType::ACTIVE ? layers_in_active_tree_
                                           : layers_in_pending_tree_;
  auto kv = layers_in_tree.find(element_id);
  if (kv == layers_in_tree.end())
    return nullptr;

  DCHECK(kv->second);
  return kv->second.get();
}

TestAnimationDelegate::TestAnimationDelegate()
    : started_(false),
      finished_(false),
      aborted_(false),
      takeover_(false),
      start_time_(base::TimeTicks()) {}

void TestAnimationDelegate::NotifyAnimationStarted(
    base::TimeTicks monotonic_time,
    int target_property,
    int group) {
  started_ = true;
  start_time_ = monotonic_time;
}

void TestAnimationDelegate::NotifyAnimationFinished(
    base::TimeTicks monotonic_time,
    int target_property,
    int group) {
  finished_ = true;
}

void TestAnimationDelegate::NotifyAnimationAborted(
    base::TimeTicks monotonic_time,
    int target_property,
    int group) {
  aborted_ = true;
}

void TestAnimationDelegate::NotifyAnimationTakeover(
    base::TimeTicks monotonic_time,
    int target_property,
    base::TimeTicks animation_start_time,
    std::unique_ptr<AnimationCurve> curve) {
  takeover_ = true;
}

AnimationTimelinesTest::AnimationTimelinesTest()
    : client_(ThreadInstance::MAIN),
      client_impl_(ThreadInstance::IMPL),
      host_(nullptr),
      host_impl_(nullptr),
      timeline_id_(AnimationIdProvider::NextTimelineId()),
      player_id_(AnimationIdProvider::NextPlayerId()),
      next_test_layer_id_(0) {
  host_ = client_.host();
  host_impl_ = client_impl_.host();

  element_id_ = ElementId(NextTestLayerId());
}

AnimationTimelinesTest::~AnimationTimelinesTest() = default;

void AnimationTimelinesTest::SetUp() {
  timeline_ = AnimationTimeline::Create(timeline_id_);
}

void AnimationTimelinesTest::TearDown() {
  host_impl_->ClearMutators();
  host_->ClearMutators();
}

void AnimationTimelinesTest::CreateTestLayer(
    bool needs_active_value_observations,
    bool needs_pending_value_observations) {
  CreateTestMainLayer();

  if (needs_pending_value_observations)
    CreateTestImplLayer(ElementListType::PENDING);
  if (needs_active_value_observations)
    CreateTestImplLayer(ElementListType::ACTIVE);
}

void AnimationTimelinesTest::CreateTestMainLayer() {
  client_.RegisterElement(element_id_, ElementListType::ACTIVE);
}

void AnimationTimelinesTest::DestroyTestMainLayer() {
  client_.UnregisterElement(element_id_, ElementListType::ACTIVE);
}

void AnimationTimelinesTest::CreateTestImplLayer(
    ElementListType element_list_type) {
  client_impl_.RegisterElement(element_id_, element_list_type);
}

void AnimationTimelinesTest::AttachTimelinePlayerLayer() {
  host_->AddAnimationTimeline(timeline_);
  timeline_->AttachPlayer(player_);
  player_->AttachElement(element_id_);

  element_animations_ = player_->animation_ticker()->element_animations();
}

void AnimationTimelinesTest::CreateImplTimelineAndPlayer() {
  host_->PushPropertiesTo(host_impl_);
  GetImplTimelineAndPlayerByID();
}

void AnimationTimelinesTest::GetImplTimelineAndPlayerByID() {
  timeline_impl_ = host_impl_->GetTimelineById(timeline_id_);
  EXPECT_TRUE(timeline_impl_);
  player_impl_ = static_cast<SingleTickerAnimationPlayer*>(
      timeline_impl_->GetPlayerById(player_id_));
  EXPECT_TRUE(player_impl_);

  element_animations_impl_ =
      player_impl_->animation_ticker()->element_animations();
}

void AnimationTimelinesTest::ReleaseRefPtrs() {
  player_ = nullptr;
  timeline_ = nullptr;
  player_impl_ = nullptr;
  timeline_impl_ = nullptr;
}

void AnimationTimelinesTest::TickAnimationsTransferEvents(
    base::TimeTicks time,
    unsigned expect_events) {
  std::unique_ptr<MutatorEvents> events = host_->CreateEvents();

  // TODO(smcgruer): Construct a proper ScrollTree for the tests.
  ScrollTree scroll_tree;
  host_impl_->TickAnimations(time, scroll_tree);
  host_impl_->UpdateAnimationState(true, events.get());

  auto* animation_events = static_cast<const AnimationEvents*>(events.get());
  EXPECT_EQ(expect_events, animation_events->events_.size());

  host_->TickAnimations(time, scroll_tree);
  host_->UpdateAnimationState(true, nullptr);
  host_->SetAnimationEvents(std::move(events));
}

AnimationTicker* AnimationTimelinesTest::GetTickerForElementId(
    ElementId element_id) {
  const scoped_refptr<ElementAnimations> element_animations =
      host_->GetElementAnimationsForElementId(element_id);
  return element_animations ? &*element_animations->tickers_list().begin()
                            : nullptr;
}

AnimationTicker* AnimationTimelinesTest::GetImplTickerForLayerId(
    ElementId element_id) {
  const scoped_refptr<ElementAnimations> element_animations =
      host_impl_->GetElementAnimationsForElementId(element_id);
  return element_animations ? &*element_animations->tickers_list().begin()
                            : nullptr;
}

int AnimationTimelinesTest::NextTestLayerId() {
  next_test_layer_id_++;
  return next_test_layer_id_;
}

bool AnimationTimelinesTest::CheckTickerTimelineNeedsPushProperties(
    bool needs_push_properties) const {
  DCHECK(player_);
  DCHECK(timeline_);

  bool result = true;

  AnimationTicker* ticker = player_->animation_ticker();
  if (ticker->needs_push_properties() != needs_push_properties) {
    ADD_FAILURE() << "ticker->needs_push_properties() expected to be "
                  << needs_push_properties;
    result = false;
  }
  if (timeline_->needs_push_properties() != needs_push_properties) {
    ADD_FAILURE() << "timeline_->needs_push_properties() expected to be "
                  << needs_push_properties;
    result = false;
  }

  return result;
}

void AnimationTimelinesTest::PushProperties() {
  host_->PushPropertiesTo(host_impl_);
}

}  // namespace cc
