// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MediaControlTimeDisplayElement_h
#define MediaControlTimeDisplayElement_h

#include "core/html/shadow/MediaControlElementTypes.h"

namespace blink {

class MediaControlsImpl;

class MediaControlTimeDisplayElement : public MediaControlDivElement {
 public:
  void SetCurrentValue(double);
  double CurrentValue() const;

 protected:
  MediaControlTimeDisplayElement(MediaControlsImpl&, MediaControlElementType);

 private:
  double current_value_ = 0;
};

}  // namespace blink
#endif  // MediaControlTimeDisplayElement_h
