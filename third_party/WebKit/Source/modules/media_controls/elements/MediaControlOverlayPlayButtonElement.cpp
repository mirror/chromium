// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/media_controls/elements/MediaControlOverlayPlayButtonElement.h"

#include "build/build_config.h"
#include "core/dom/ElementShadow.h"
#include "core/dom/events/Event.h"
#include "core/events/MouseEvent.h"
#include "core/geometry/DOMRect.h"
#include "core/html/HTMLStyleElement.h"
#include "core/html/media/HTMLMediaElement.h"
#include "core/html/media/HTMLMediaSource.h"
#include "core/input_type_names.h"
#include "modules/media_controls/MediaControlsImpl.h"
#include "modules/media_controls/MediaControlsResourceLoader.h"
#include "modules/media_controls/elements/MediaControlElementsHelper.h"
#include "platform/runtime_enabled_features.h"
#include "platform/wtf/Time.h"
#include "public/platform/Platform.h"
#include "public/platform/TaskType.h"
#include "public/platform/WebSize.h"

namespace {

// The size of the inner circle button in pixels.
constexpr int kInnerButtonSize = 56;

// The touch padding of the inner circle button in pixels.
constexpr int kInnerButtonTouchPaddingSize = 20;

// Check if a point is based within the boundary of a DOMRect with a margin.
bool IsPointInRect(blink::DOMRect& rect, int margin, int x, int y) {
  return ((x >= rect.left() - margin) && (x <= (rect.right() + margin)) &&
          (y >= rect.top() - margin) && (y <= (rect.bottom() + margin)));
}

// The delay if a touch is outside the internal button.
constexpr WTF::TimeDelta kOutsideTouchDelay = TimeDelta::FromMilliseconds(300);

// The delay if a touch is inside the internal button.
constexpr WTF::TimeDelta kInsideTouchDelay = TimeDelta::FromMilliseconds(0);

#if defined(OS_ANDROID)
// The number of seconds to jump when double tapping.
constexpr int kNumberOfSecondsToJump = 10;
#endif

}  // namespace.

namespace blink {

MediaControlOverlayPlayButtonElement::AnimatedArrow::AnimatedArrow(
    const AtomicString& id,
    ContainerNode& parent)
    : HTMLDivElement(parent.GetDocument()),
      last_arrow(nullptr),
      svg_container(nullptr),
      event_listener_(nullptr) {
  setAttribute("id", id);
  parent.AppendChild(this);

  SetInnerHTMLFromString(MediaControlsResourceLoader::GetJumpSVGImage());

  last_arrow = getElementById("arrow-3");
  svg_container = getElementById("jump");

  event_listener_ = new MediaControlAnimationEventListener(this);
  svg_container->SetInlineStyleProperty(CSSPropertyDisplay, CSSValueNone);
}

void MediaControlOverlayPlayButtonElement::AnimatedArrow::
    OnAnimationIteration() {
  counter_--;

  if (counter_ == 0) {
    svg_container->SetInlineStyleProperty(CSSPropertyDisplay, CSSValueNone);
    hidden_ = true;
  }
}

void MediaControlOverlayPlayButtonElement::AnimatedArrow::Show() {
  if (hidden_)
    svg_container->RemoveInlineStyleProperty(CSSPropertyDisplay);

  counter_++;
}

Element&
MediaControlOverlayPlayButtonElement::AnimatedArrow::WatchedAnimationElement()
    const {
  return *last_arrow;
}

void MediaControlOverlayPlayButtonElement::AnimatedArrow::Trace(
    Visitor* visitor) {
  MediaControlAnimationEventListener::Observer::Trace(visitor);
  HTMLDivElement::Trace(visitor);
  visitor->Trace(last_arrow);
  visitor->Trace(svg_container);
  visitor->Trace(event_listener_);
}

// The DOM structure looks like:
//
// MediaControlOverlayPlayButtonElement
//   (-webkit-media-controls-overlay-play-button)
// +-div (-internal-media-controls-overlay-play-button-internal)
//   {if MediaControlsImpl::IsModern}
//   This contains the inner circle with the actual play/pause icon.
MediaControlOverlayPlayButtonElement::MediaControlOverlayPlayButtonElement(
    MediaControlsImpl& media_controls)
    : MediaControlInputElement(media_controls, kMediaOverlayPlayButton),
      tap_timer_(GetDocument().GetTaskRunner(TaskType::kMediaElementEvent),
                 this,
                 &MediaControlOverlayPlayButtonElement::TapTimerFired),
      internal_button_(nullptr),
      left_jump_arrow_(nullptr),
      right_jump_arrow_(nullptr) {
  EnsureUserAgentShadowRoot();
  setType(InputTypeNames::button);
  SetShadowPseudoId(AtomicString("-webkit-media-controls-overlay-play-button"));

  if (MediaControlsImpl::IsModern()) {
    ShadowRoot& shadow_root = Shadow()->OldestShadowRoot();

    left_jump_arrow_ = new MediaControlOverlayPlayButtonElement::AnimatedArrow(
        "left-arrow", shadow_root);

    internal_button_ = MediaControlElementsHelper::CreateDiv(
        "-internal-media-controls-overlay-play-button-internal", &shadow_root);

    // This stylesheet element and will contain rules that are specific to the
    // loading panel. The shadow DOM protects these rules and rules from the
    // parent DOM from bleeding across the shadow DOM boundary.
    HTMLStyleElement* style = HTMLStyleElement::Create(GetDocument(), false);
    style->setTextContent(
        MediaControlsResourceLoader::GetOverlayPlayStyleSheet());
    shadow_root.AppendChild(style);

    right_jump_arrow_ = new MediaControlOverlayPlayButtonElement::AnimatedArrow(
        "right-arrow", shadow_root);
  }
}

void MediaControlOverlayPlayButtonElement::UpdateDisplayType() {
  SetIsWanted(MediaElement().ShouldShowControls() &&
              (MediaControlsImpl::IsModern() || MediaElement().paused()));
  MediaControlInputElement::UpdateDisplayType();
}

const char* MediaControlOverlayPlayButtonElement::GetNameForHistograms() const {
  return "PlayOverlayButton";
}

void MediaControlOverlayPlayButtonElement::MaybePlayPause() {
  if (MediaElement().paused()) {
    Platform::Current()->RecordAction(
        UserMetricsAction("Media.Controls.PlayOverlay"));
  } else {
    Platform::Current()->RecordAction(
        UserMetricsAction("Media.Controls.PauseOverlay"));
  }

  // Allow play attempts for plain src= media to force a reload in the error
  // state. This allows potential recovery for transient network and decoder
  // resource issues.
  const String& url = MediaElement().currentSrc().GetString();
  if (MediaElement().error() && !HTMLMediaElement::IsMediaStreamURL(url) &&
      !HTMLMediaSource::Lookup(url)) {
    MediaElement().load();
  }

  if (MediaElement().paused()) {
    // Bypasses user gesture check because the gesture may not exist anymore
    // when this function is triggered by the timer.
    MediaElement().PlayInternal();
  } else {
    MediaElement().PauseInternal();
  }

  UpdateDisplayType();
}

void MediaControlOverlayPlayButtonElement::MaybeJump(int seconds) {
  double new_time = std::max(0.0, MediaElement().currentTime() + seconds);
  new_time = std::min(new_time, MediaElement().duration());
  MediaElement().setCurrentTime(new_time);

  if (seconds > 0)
    right_jump_arrow_->Show();
  else
    left_jump_arrow_->Show();
}

void MediaControlOverlayPlayButtonElement::HandlePlayPauseEvent(
    Event* event,
    WTF::TimeDelta delay) {
  event->SetDefaultHandled();

  if (tap_timer_.IsActive())
    return;

  tap_timer_.StartOneShot(delay, BLINK_FROM_HERE);
}

void MediaControlOverlayPlayButtonElement::DefaultEventHandler(Event* event) {
  if (event->type() == EventTypeNames::click) {
    // Double tap to navigate should only be available on modern controls.
    if (!MediaControlsImpl::IsModern() || !event->IsMouseEvent()) {
      HandlePlayPauseEvent(event, kInsideTouchDelay);
      return;
    }

    // If the event doesn't have position data we should just default to
    // play/pause.
    MouseEvent* mouse_event = ToMouseEvent(event);
    if (!mouse_event->HasPosition()) {
      HandlePlayPauseEvent(event, kInsideTouchDelay);
      return;
    }

    // If the click happened on the internal button or a margin around it then
    // we should play/pause.
    if (IsPointInRect(*internal_button_->getBoundingClientRect(),
                      kInnerButtonTouchPaddingSize, mouse_event->clientX(),
                      mouse_event->clientY())) {
      HandlePlayPauseEvent(event, kInsideTouchDelay);
    } else if (!tap_timer_.IsActive()) {
      // If there was not a previous touch and this was outside of the button
      // then we should play/pause but with a small unnoticeable delay to allow
      // for a secondary tap.
      HandlePlayPauseEvent(event, kOutsideTouchDelay);
    } else {
      // Cancel the play pause event.
      tap_timer_.Stop();

#if defined(OS_ANDROID)
      // If the double tap happened on Android, then jump forwards or backwards
      // based on the position of the tap.
      WebSize element_size =
          MediaControlElementsHelper::GetSizeOrDefault(*this, WebSize(0, 0));

      if (mouse_event->clientX() >= element_size.width / 2) {
        MaybeJump(kNumberOfSecondsToJump);
      } else {
        MaybeJump(kNumberOfSecondsToJump * -1);
      }
#else
      // If the double tap happened on any other platform, then toggle
      // fullscreen.
      if (MediaElement().IsFullscreen())
        GetMediaControls().ExitFullscreen();
      else
        GetMediaControls().EnterFullscreen();
#endif

      event->SetDefaultHandled();
    }
  }

  // TODO(mlamouri): should call MediaControlInputElement::DefaultEventHandler.
}

bool MediaControlOverlayPlayButtonElement::KeepEventInNode(Event* event) {
  return MediaControlElementsHelper::IsUserInteractionEvent(event);
}

WebSize MediaControlOverlayPlayButtonElement::GetSizeOrDefault() const {
  // The size should come from the internal button which actually displays the
  // button.
  return MediaControlElementsHelper::GetSizeOrDefault(
      *internal_button_, WebSize(kInnerButtonSize, kInnerButtonSize));
}

void MediaControlOverlayPlayButtonElement::TapTimerFired(TimerBase*) {
  MaybePlayPause();
}

void MediaControlOverlayPlayButtonElement::Trace(blink::Visitor* visitor) {
  MediaControlInputElement::Trace(visitor);
  visitor->Trace(internal_button_);
  visitor->Trace(left_jump_arrow_);
  visitor->Trace(right_jump_arrow_);
}

}  // namespace blink
