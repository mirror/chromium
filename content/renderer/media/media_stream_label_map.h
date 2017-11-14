// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_MEDIA_STREAM_LABEL_MAP_H_
#define CONTENT_RENDERER_MEDIA_MEDIA_STREAM_LABEL_MAP_H_

#include <map>
#include <string>

#include "content/public/common/media_stream_request.h"

namespace content {

class MediaStreamLabelMap {
 public:
  struct Stream {
    Stream();
    ~Stream();
    MediaStreamDevices audio_devices;
    MediaStreamDevices video_devices;
  };

  MediaStreamLabelMap();
  ~MediaStreamLabelMap();

  void CreateStream(const std::string& label, const MediaStreamDevice& device);
  Stream* GetStream(const std::string& label);
  bool RemoveStream(const std::string& label);
  MediaStreamDevices GetNonScreenCaptureDevices();

 private:
  using StreamMap = std::map<std::string, Stream>;
  StreamMap streams_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_MEDIA_STREAM_LABEL_MAP_H_