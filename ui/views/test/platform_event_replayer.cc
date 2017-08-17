// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/test/platform_event_replayer.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "build/build_config.h"
#include "ui/events/platform/platform_event_source.h"
#include "ui/events/test/platform_event_source_test_api.h"

#if defined(OS_LINUX) && defined(USE_X11)
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#endif

namespace ui {

class BinaryDeserializer {
 public:
  BinaryDeserializer() {}
  ~BinaryDeserializer() { delete[] buffer_; }

  // Read in the data stored in the respective file and store in a buffer.
  bool ReadFromFile(const std::string& filename) {
    std::ifstream infile;
    infile.open(filename.c_str(), std::ios::binary);
    infile.seekg(0, infile.end);
    size_ = infile.tellg();
    infile.seekg(0);
    buffer_ = new char[size_];
    infile.read(buffer_, size_);
    bool success = !(infile.failbit | infile.badbit);
    infile.close();
    return success;
  }

  // Parse the buffer and dispatch event to the PlatformEventSource.
  bool DeserializeAndDispatchFromFile() {
#if defined(OS_LINUX) && defined(USE_X11)
    std::unique_ptr<test::PlatformEventSourceTestAPI> platform_event_source =
        base::MakeUnique<test::PlatformEventSourceTestAPI>(
            PlatformEventSource::GetInstance());

    for (long i = 0; i < size_;) {
      XEvent* new_xev = static_cast<XEvent*>((void*)(buffer_ + i));
      i += sizeof(XEvent);
      if (new_xev->type == GenericEvent) {
        new_xev->xcookie.data = (void*)(buffer_ + i);
        i += sizeof(XIDeviceEvent);
      }
      platform_event_source->Dispatch(new_xev);
    }
    return true;
#else
    return false;
#endif
  }

 private:
  char* buffer_;
  long size_;
};

PlatformEventReplayer::PlatformEventReplayer()
    : deserializer_(new BinaryDeserializer()) {}

PlatformEventReplayer::~PlatformEventReplayer() {}

bool PlatformEventReplayer::ReadInPlatformEvents(const std::string filename) {
  return deserializer_->ReadFromFile(filename);
}

bool PlatformEventReplayer::ReplayPlatformEvents() {
  return deserializer_->DeserializeAndDispatchFromFile();
}

}  // namespace ui
