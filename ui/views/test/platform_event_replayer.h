// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_PLATFORM_PLATFORM_EVENT_REPLAYER_H_
#define UI_EVENTS_PLATFORM_PLATFORM_EVENT_REPLAYER_H_

#include <fstream>

namespace ui {

class BinaryDeserializer;

class PlatformEventReplayer {
 public:
  PlatformEventReplayer();
  ~PlatformEventReplayer();

  bool ReplayPlatformEvents();
  bool ReadInPlatformEvents(const std::string filename);

 private:
  std::unique_ptr<BinaryDeserializer> deserializer_;
};

}  // namespace ui

#endif
