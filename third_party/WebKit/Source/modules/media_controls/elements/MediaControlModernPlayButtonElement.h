// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MediaControlModernPlayButtonElement_h
#define MediaControlModernPlayButtonElement_h

#include "modules/media_controls/elements/MediaControlInputElement.h"

namespace blink {

class Event;
class MediaControlsImpl;

class MediaControlModernPlayButtonElement final
    : public MediaControlInputElement {
 public:
  explicit MediaControlModernPlayButtonElement(MediaControlsImpl&);

  // MediaControlInputElement overrides.
  bool WillRespondToMouseClickEvents() final;
  void UpdateDisplayType() final;
  bool HasOverflowButton() const final;

  void OnMediaKeyboardEvent(Event* event) { DefaultEventHandler(event); }

  WebSize GetSizeOrDefault() const final;

 protected:
  const char* GetNameForHistograms() const final;

 private:
  void DefaultEventHandler(Event*) final;
};

}  // namespace blink

#endif  // MediaControlModernPlayButtonElement_h
