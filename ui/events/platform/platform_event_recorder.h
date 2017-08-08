// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_PLATFORM_PLATFORM_EVENT_RECORDER_H_
#define UI_EVENTS_PLATFORM_PLATFORM_EVENT_RECORDER_H_

#include <fstream>
#include "ui/events/platform/platform_event_types.h"

namespace ui {

class BinarySerializer;

class PlatformEventRecorder {
 public:
  PlatformEventRecorder();
  ~PlatformEventRecorder();

  bool RecordPlatformEvent(PlatformEvent& event);

 private:
  std::unique_ptr<BinarySerializer> serializer_;
};

class BinarySerializer {
 public:
  BinarySerializer();
  ~BinarySerializer();

  std::string GenerateFilename();
  bool SerializeToFile(PlatformEvent& event);

 private:
  std::ofstream file;
  std::string filename;
};

}  // namespace ui

#endif
