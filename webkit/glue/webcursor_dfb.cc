// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/glue/webcursor.h"

#include "base/logging.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebCursorInfo.h"

void WebCursor::InitPlatformData() {
  NOTIMPLEMENTED();
}

bool WebCursor::SerializePlatformData(Pickle* pickle) const {
  NOTIMPLEMENTED();
  return true;
}

bool WebCursor::DeserializePlatformData(PickleIterator* iter) {
  NOTIMPLEMENTED();
  return true;
}

bool WebCursor::IsPlatformDataEqual(const WebCursor& other) const {
  NOTIMPLEMENTED();
  return true;
}

void WebCursor::CleanupPlatformData() {
  NOTIMPLEMENTED();
}

void WebCursor::CopyPlatformData(const WebCursor& other) {
  NOTIMPLEMENTED();
}
