// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media/media_stream_label_map.h"

#include "base/logging.h"

namespace content {

MediaStreamLabelMap::MediaStreamLabelMap() {}

MediaStreamLabelMap::~MediaStreamLabelMap() {}

void MediaStreamLabelMap::CreateStream(const std::string& label,
                                       const MediaStreamDevice& device) {
  Stream stream;
  if (IsAudioInputMediaType(device.type))
    stream.audio_devices.push_back(device);
  else if (IsVideoMediaType(device.type))
    stream.video_devices.push_back(device);
  else
    NOTREACHED();
  streams_[label] = stream;
}

MediaStreamLabelMap::Stream* MediaStreamLabelMap::GetStream(
    const std::string& label) {
  StreamMap::iterator it = streams_.find(label);
  return it == streams_.end() ? nullptr : &it->second;
}

bool MediaStreamLabelMap::RemoveStream(const std::string& label) {
  StreamMap::iterator it = streams_.find(label);
  if (it == streams_.end())
    return false;

  streams_.erase(it);
  return true;
}

MediaStreamDevices MediaStreamLabelMap::GetNonScreenCaptureDevices() {
  MediaStreamDevices video_devices;
  for (const auto& stream_it : streams_) {
    for (const auto& video_device : stream_it.second.video_devices) {
      if (!IsScreenCaptureMediaType(video_device.type))
        video_devices.push_back(video_device);
    }
  }
  return video_devices;
}

MediaStreamLabelMap::Stream::Stream() {}

MediaStreamLabelMap::Stream::~Stream() {}

}  // namespace content
