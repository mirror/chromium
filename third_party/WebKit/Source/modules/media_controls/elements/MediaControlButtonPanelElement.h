// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MediaControlButtonPanelElement_h
#define MediaControlButtonPanelElement_h

#include "modules/media_controls/elements/MediaControlDivElement.h"

namespace blink {

class MediaControlsImpl;

class MediaControlButtonPanelElement final : public MediaControlDivElement {
 public:
  explicit MediaControlButtonPanelElement(MediaControlsImpl&);
};

}  // namespace blink

#endif  // MediaControlButtonPanelElement_h
