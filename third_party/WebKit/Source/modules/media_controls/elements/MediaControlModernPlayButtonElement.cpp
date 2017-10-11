// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/media_controls/elements/MediaControlModernPlayButtonElement.h"

#include "core/InputTypeNames.h"
#include "core/dom/ElementShadow.h"
#include "core/dom/events/Event.h"
#include "core/html/HTMLMediaElement.h"
#include "core/html/media/HTMLMediaSource.h"
#include "modules/media_controls/MediaControlsImpl.h"
#include "public/platform/Platform.h"

namespace blink {

// The DOM structure looks like:
//
// MediaControlModernPlayButtonElement
//   (-internal-media-controls-play-button)
// +-div (-internal-media-controls-play-button-internal)
MediaControlModernPlayButtonElement::MediaControlModernPlayButtonElement(
    MediaControlsImpl& media_controls)
    : MediaControlInputElement(media_controls, kMediaPlayButton) {
  EnsureUserAgentShadowRoot();
  setType(InputTypeNames::button);
  SetShadowPseudoId(AtomicString("-internal-media-controls-play-button"));

  ShadowRoot& shadow_root = Shadow()->OldestShadowRoot();
  CreateDiv("-internal-media-controls-play-button-internal", &shadow_root);
}

bool MediaControlModernPlayButtonElement::WillRespondToMouseClickEvents() {
  return true;
}

void MediaControlModernPlayButtonElement::UpdateDisplayType() {
  SetDisplayType(MediaElement().paused() ? kMediaPlayButton
                                         : kMediaPauseButton);
  SetClass("pause", MediaElement().paused());
  UpdateOverflowString();
}

bool MediaControlModernPlayButtonElement::HasOverflowButton() const {
  return false;
}

const char* MediaControlModernPlayButtonElement::GetNameForHistograms() const {
  return "PlayPauseButton";
}

void MediaControlModernPlayButtonElement::DefaultEventHandler(Event* event) {
  if (event->type() == EventTypeNames::click) {
    if (MediaElement().paused()) {
      Platform::Current()->RecordAction(
          UserMetricsAction("Media.Controls.Play"));
    } else {
      Platform::Current()->RecordAction(
          UserMetricsAction("Media.Controls.Pause"));
    }

    // Allow play attempts for plain src= media to force a reload in the error
    // state. This allows potential recovery for transient network and decoder
    // resource issues.
    const String& url = MediaElement().currentSrc().GetString();
    if (MediaElement().error() && !HTMLMediaElement::IsMediaStreamURL(url) &&
        !HTMLMediaSource::Lookup(url))
      MediaElement().load();

    MediaElement().TogglePlayState();
    UpdateDisplayType();
    event->SetDefaultHandled();
  }
  MediaControlInputElement::DefaultEventHandler(event);
}

}  // namespace blink
