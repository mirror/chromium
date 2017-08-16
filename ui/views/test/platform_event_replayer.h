// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_PLATFORM_PLATFORM_EVENT_REPLAYER_H_
#define UI_EVENTS_PLATFORM_PLATFORM_EVENT_REPLAYER_H_

#include <fstream>

namespace ui {

class BinaryDeserializer;

// A tool that is initialized in a test case. It reads in data from a
// specified file and reforms this data in PlatformEvents which are then
// dispatched by the PlatformEventSource. Files of PlatformEvents can be
// recorded for playback using the PlatformEventRecorder class that is
// declared in ui/events/platform/platform_event_recorder.h.
class PlatformEventReplayer {
 public:
  PlatformEventReplayer();
  ~PlatformEventReplayer();

  // Reads all of the PlatformEvent data from file into a buffer in
  // the deserialization class
  bool ReadInPlatformEvents(const std::string filename);

  // Dispatches all of the PlatfromEvents that were read in from the file
  // to the PlatformEventSource.
  bool ReplayPlatformEvents();

 private:
  std::unique_ptr<BinaryDeserializer> deserializer_;
};

}  // namespace ui

#endif
