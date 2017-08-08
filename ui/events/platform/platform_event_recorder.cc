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

PlatformEventRecorder::PlatformEventRecorder()
    : serializer_(new BinarySerializer()) {}

PlatformEventRecorder::~PlatformEventRecorder() {}

bool PlatformEventRecorder::RecordPlatformEvent(PlatformEvent& event) {
  return serializer_->SerializeToFile(event);
}

BinarySerializer::BinarySerializer() : filename(GenerateFilename()) {
  file.open(filename.c_str(), std::ios::binary | std::ofstream::app);
}

BinarySerializer::~BinarySerializer() {
  file.close();
  LOG(ERROR) << "Events recorded to : " << filename;
}

std::string BinarySerializer::GenerateFilename() {
  base::Time current_time = base::Time::NowFromSystemTime();
  base::Time::Exploded ex;
  current_time.UTCExplode(&ex);

  std::string file = "ui/views/test/recorder_files/";
  file += std::to_string(ex.year) + "_" + std::to_string(ex.month) + "_" +
          std::to_string(ex.day_of_month) + "_" + std::to_string(ex.hour) +
          "_" + std::to_string(ex.minute) + "_" + std::to_string(ex.second) +
          ".bin";

  return file;
}

bool BinarySerializer::SerializeToFile(PlatformEvent& event) {
#if defined(OS_CHROMEOS)
#elif defined(OS_LINUX)
  XEvent* xev = event;
  file.write((char*)xev, sizeof(XEvent));
  if (xev->type == GenericEvent)
    file.write((char*)xev->xcookie.data, sizeof(XIDeviceEvent));
  return true;
#endif
}

}  // namespace ui
