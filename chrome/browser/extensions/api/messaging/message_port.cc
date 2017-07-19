// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/messaging/message_port.h"

namespace extensions {

void MessagePort::RemoveCommonFrames(const MessagePort& port) {}

bool MessagePort::HasFrame(content::RenderFrameHost* rfh) const {
  return false;
}

}  // namespace extensions
