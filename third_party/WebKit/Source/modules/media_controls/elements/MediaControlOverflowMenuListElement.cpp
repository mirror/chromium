// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/media_controls/elements/MediaControlOverflowMenuListElement.h"

#include "core/dom/events/Event.h"
#include "modules/media_controls/MediaControlsImpl.h"
#include "platform/Histogram.h"

namespace blink {

MediaControlOverflowMenuListElement::MediaControlOverflowMenuListElement(
    MediaControlsImpl& media_controls)
    : MediaControlDivElement(media_controls, kMediaOverflowList) {
  SetShadowPseudoId(
      AtomicString("-internal-media-controls-overflow-menu-list"));
  SetIsWanted(false);
}

void MediaControlOverflowMenuListElement::DefaultEventHandler(Event* event) {
  if (event->type() == EventTypeNames::click)
    event->SetDefaultHandled();

  MediaControlDivElement::DefaultEventHandler(event);
}

void MediaControlOverflowMenuListElement::SetIsWanted(bool wanted) {
  MediaControlDivElement::SetIsWanted(wanted);

  // Record the time the overflow menu was shown to a histogram.
  if (wanted) {
    DCHECK(!time_shown_);
    time_shown_ = TimeTicks::Now();
  } else if (time_shown_) {
    WTF::TimeDelta time_taken = TimeTicks::Now() - time_shown_.value();
    DEFINE_STATIC_LOCAL(LinearHistogram, histogram,
                        ("Media.Controls.Overflow.TimeToAction", 1, 100, 100));
    histogram.Count(static_cast<int32_t>(time_taken.InSeconds()));
    time_shown_.reset();
  }
}

}  // namespace blink
