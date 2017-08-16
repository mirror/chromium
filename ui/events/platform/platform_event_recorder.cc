// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/platform/platform_event_recorder.h"

#include "base/logging.h"
#include "base/time/time.h"
#include "build/build_config.h"

#if defined(OS_LINUX) && defined(USE_X11)
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#endif

namespace ui {

class BinarySerializer {
 public:
  BinarySerializer() : filename_(GenerateFilename()) {
    file_.open(filename_.c_str(), std::ios::binary | std::ofstream::app);
  }

  // Print the name of the file when recording is done so that the filename
  // can be used as input to the replaying tool.
  ~BinarySerializer() {
    file_.close();
    LOG(ERROR) << "Events recorded to : " << filename_;
  }

  // Generate a filename based on the UTC timestamp.
  std::string GenerateFilename() {
    base::Time current_time = base::Time::NowFromSystemTime();
    base::Time::Exploded ex;
    current_time.UTCExplode(&ex);

    std::string filename = "ui/views/test/recorder_files/";
    filename += std::to_string(ex.year) + "_" + std::to_string(ex.month) + "_" +
                std::to_string(ex.day_of_month) + "_" +
                std::to_string(ex.hour) + "_" + std::to_string(ex.minute) +
                "_" + std::to_string(ex.second) + ".txt";

    return filename;
  }

  // Serialize the event and write it to the file. Only works for Linux X11
  // right now. Add cases for other platforms in the future.
  bool SerializeToFile(const PlatformEvent& event) {
#if defined(OS_LINUX) && defined(USE_X11)
    XEvent* xev = event;
    file_.write((char*)xev, sizeof(XEvent));
    if (xev->type == GenericEvent)
      file_.write((char*)xev->xcookie.data, sizeof(XIDeviceEvent));
    return true;
#else
    return false;
#endif
  }

 private:
  std::ofstream file_;
  std::string filename_;
};

PlatformEventRecorder::PlatformEventRecorder()
    : serializer_(new BinarySerializer()) {}

PlatformEventRecorder::~PlatformEventRecorder() {}

bool PlatformEventRecorder::RecordPlatformEvent(const PlatformEvent& event) {
  return serializer_->SerializeToFile(event);
}

}  // namespace ui
