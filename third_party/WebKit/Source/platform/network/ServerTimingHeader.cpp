// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/network/ServerTimingHeader.h"

namespace blink {

void ServerTimingHeader::SetParam(String name, String value) {
  if (name == "duration") {
    if (!durationSet_) {
      duration_ = value.ToDouble();
      durationSet_ = true;
    }
  } else if (name == "description") {
    if (!descriptionSet_) {
      description_ = value;
      descriptionSet_ = true;
    }
  }
}

}  // namespace blink
