// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/animationworklet/WorkletAnimation.h"

#include "core/animation/ElementAnimations.h"
#include "core/animation/KeyframeEffectModel.h"
#include "core/animation/ScrollTimeline.h"
#include "core/animation/Timing.h"
#include "core/dom/Node.h"
#include "core/dom/NodeComputedStyle.h"
#include "core/layout/LayoutBox.h"
#include "platform/wtf/text/WTFString.h"
#include "public/platform/Platform.h"
#include "public/platform/WebCompositorSupport.h"

namespace blink {

namespace {
bool ValidateEffects(const HeapVector<Member<KeyframeEffectReadOnly>>& effects,
                     String& error_string) {
  if (effects.IsEmpty()) {
    error_string = "Effects array must be non-empty";
    return false;
  }

  Document& target_document = effects.at(0)->Target()->GetDocument();
  for (const auto& effect : effects) {
    if (effect->Target()->GetDocument() != target_document) {
      error_string = "All effects must target elements in the same document";
      return false;
    }
  }
  return true;
}

bool ValidateTimeline(const DocumentTimelineOrScrollTimeline& timeline,
                      String& error_string) {
  if (timeline.IsScrollTimeline()) {
    DoubleOrScrollTimelineAutoKeyword time_range;
    timeline.GetAsScrollTimeline()->timeRange(time_range);
    if (time_range.IsScrollTimelineAutoKeyword()) {
      error_string = "ScrollTimeline timeRange must have non-auto value";
      return false;
    }
  }
  return true;
}

CompositorElementId GetCompositorScrollElementId(const Element& element) {
  DCHECK(element.GetLayoutObject());
  DCHECK(element.GetLayoutObject()->HasLayer());
  return CompositorElementIdFromUniqueObjectId(
      element.GetLayoutObject()->UniqueId(),
      CompositorElementIdNamespace::kScroll);
}

// Convert the blink concept of a ScrollTimeline orientation into the cc one.
//
// The compositor does not know about writing modes, so we have to convert the
// web concepts of 'block' and 'inline' direction into absolute vertical or
// horizontal directions.
//
// TODO(smcgruer): If the writing mode of a scroller changes, we have to update
// any related cc::ScrollTimeline somehow.
CompositorScrollTimeline::ScrollDirection ConvertOrientation(
    ScrollTimeline::ScrollDirection orientation,
    bool is_horizontal_writing_mode) {
  switch (orientation) {
    case ScrollTimeline::Block:
      return is_horizontal_writing_mode ? CompositorScrollTimeline::Vertical
                                        : CompositorScrollTimeline::Horizontal;
    case ScrollTimeline::Inline:
      return is_horizontal_writing_mode ? CompositorScrollTimeline::Horizontal
                                        : CompositorScrollTimeline::Vertical;
    default:
      NOTREACHED();
      return CompositorScrollTimeline::Vertical;
  }
}

// Converts a blink::ScrollTimeline into a cc::ScrollTimeline.
//
// If the timeline cannot be converted, returns nullptr.
std::unique_ptr<CompositorScrollTimeline> ToCompositorScrollTimeline(
    const DocumentTimelineOrScrollTimeline& timeline) {
  if (!timeline.IsScrollTimeline())
    return nullptr;

  ScrollTimeline* scroll_timeline = timeline.GetAsScrollTimeline();

  // Make sure that layout is clean so that we know whether or not the
  // scrollSource has a layout box.
  //
  // TODO(smcgruer): This is probably the wrong approach. The ScrollTimeline
  // spec allows for the scrollSource to 'not currently have a CSS layout box',
  // so the compositor should be able to handle that case (and somehow update it
  // later once a layout box is created).
  Element* scroll_source = scroll_timeline->scrollSource();
  scroll_source->GetDocument()
      .UpdateStyleAndLayoutIgnorePendingStylesheetsForNode(scroll_source);
  CompositorElementId element_id = GetCompositorScrollElementId(*scroll_source);

  DoubleOrScrollTimelineAutoKeyword time_range;
  scroll_timeline->timeRange(time_range);
  // TODO(smcgruer): Handle 'auto' time range value.
  DCHECK(time_range.IsDouble());

  LayoutBox* box = scroll_source->GetLayoutBox();
  DCHECK(box);
  CompositorScrollTimeline::ScrollDirection orientation = ConvertOrientation(
      scroll_timeline->GetOrientation(), box->IsHorizontalWritingMode());

  return std::make_unique<CompositorScrollTimeline>(element_id, orientation,
                                                    time_range.GetAsDouble());
}
}  // namespace

WorkletAnimation* WorkletAnimation::Create(
    String animator_name,
    const HeapVector<Member<KeyframeEffectReadOnly>>& effects,
    DocumentTimelineOrScrollTimeline timeline,
    scoped_refptr<SerializedScriptValue> options,
    ExceptionState& exception_state) {
  DCHECK(IsMainThread());

  String error_string;
  if (!ValidateEffects(effects, error_string)) {
    exception_state.ThrowDOMException(kNotSupportedError, error_string);
    return nullptr;
  }

  if (!ValidateTimeline(timeline, error_string)) {
    exception_state.ThrowDOMException(kNotSupportedError, error_string);
    return nullptr;
  }

  Document& document = effects.at(0)->Target()->GetDocument();
  WorkletAnimation* animation = new WorkletAnimation(
      animator_name, document, effects, timeline, std::move(options));
  if (CompositorAnimationTimeline* compositor_timeline =
          document.Timeline().CompositorTimeline())
    compositor_timeline->PlayerAttached(*animation);

  return animation;
}

WorkletAnimation::WorkletAnimation(
    const String& animator_name,
    Document& document,
    const HeapVector<Member<KeyframeEffectReadOnly>>& effects,
    DocumentTimelineOrScrollTimeline timeline,
    scoped_refptr<SerializedScriptValue> options)
    : animator_name_(animator_name),
      play_state_(Animation::kIdle),
      document_(document),
      effects_(effects),
      timeline_(timeline),
      options_(std::move(options)) {
  DCHECK(IsMainThread());
  DCHECK(Platform::Current()->IsThreadedAnimationEnabled());
  DCHECK(Platform::Current()->CompositorSupport());

  compositor_player_ = CompositorAnimationPlayer::CreateWorkletPlayer(
      animator_name_, ToCompositorScrollTimeline(timeline_));
  compositor_player_->SetAnimationDelegate(this);
}

String WorkletAnimation::playState() {
  DCHECK(IsMainThread());
  return Animation::PlayStateString(play_state_);
}

void WorkletAnimation::play() {
  DCHECK(IsMainThread());
  if (play_state_ != Animation::kPending) {
    document_->GetWorkletAnimationController().AttachAnimation(*this);
    play_state_ = Animation::kPending;
  }
}

void WorkletAnimation::cancel() {
  DCHECK(IsMainThread());
  if (play_state_ != Animation::kIdle) {
    document_->GetWorkletAnimationController().DetachAnimation(*this);
    play_state_ = Animation::kIdle;
  }
}

bool WorkletAnimation::StartOnCompositor() {
  DCHECK(IsMainThread());
  Element& target = *effects_.at(0)->Target();

  // TODO(smcgruer): Creating a WorkletAnimaiton should be a hint to blink
  // compositing that the animated element should be promoted. Otherwise this
  // fails. http://crbug.com/776533
  CompositorAnimations::AttachCompositedLayers(target,
                                               compositor_player_.get());

  double animation_playback_rate = 1;
  ToKeyframeEffectModelBase(effects_.at(0)->Model())
      ->SnapshotAllCompositorKeyframes(target, target.ComputedStyleRef(),
                                       target.ParentComputedStyle());

  bool success =
      effects_.at(0)
          ->CheckCanStartAnimationOnCompositor(animation_playback_rate)
          .Ok();

  if (success) {
    double start_time = std::numeric_limits<double>::quiet_NaN();
    double time_offset = 0;
    int group = 0;

    // TODO(smcgruer): We need to start all of the effects, not just the first.
    effects_.at(0)->StartAnimationOnCompositor(group, start_time, time_offset,
                                               animation_playback_rate,
                                               compositor_player_.get());
  }

  play_state_ = success ? Animation::kRunning : Animation::kIdle;
  return success;
}

void WorkletAnimation::Dispose() {
  DCHECK(IsMainThread());

  if (CompositorAnimationTimeline* compositor_timeline =
          document_->Timeline().CompositorTimeline())
    compositor_timeline->PlayerDestroyed(*this);
  compositor_player_->SetAnimationDelegate(nullptr);
  compositor_player_ = nullptr;
}

void WorkletAnimation::Trace(blink::Visitor* visitor) {
  visitor->Trace(document_);
  visitor->Trace(effects_);
  visitor->Trace(timeline_);
  WorkletAnimationBase::Trace(visitor);
}

}  // namespace blink
