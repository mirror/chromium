// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_PLATFORM_PLATFORM_EVENT_RECORDER_H_
#define UI_EVENTS_PLATFORM_PLATFORM_EVENT_RECORDER_H_

#include <fstream>
#include "ui/events/platform/platform_event_types.h"

namespace ui {

class BinarySerializer;

// A tool that is initialized in PlatformEventSource. It records
// PlatformEvents by serializing the events and writing them to a file.
// The events can then be replayed as part of a test. To use this feature
// run chrome with --enable-event-recording and turn recording on/off with
// the hotkey Ctrl + Shift + x.
class PlatformEventRecorder {
 public:
  PlatformEventRecorder();
  ~PlatformEventRecorder();

  bool RecordPlatformEvent(const PlatformEvent& event);

 private:
  std::unique_ptr<BinarySerializer> serializer_;
};

}  // namespace ui

#endif
