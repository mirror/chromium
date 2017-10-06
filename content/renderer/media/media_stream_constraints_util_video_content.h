// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_MEDIA_STREAM_CONSTRAINTS_UTIL_VIDEO_CONTENT_H_
#define CONTENT_RENDERER_MEDIA_MEDIA_STREAM_CONSTRAINTS_UTIL_VIDEO_CONTENT_H_

#include <string>

#include "content/common/content_export.h"
#include "content/renderer/media/media_stream_constraints_util.h"
#include "third_party/webrtc/api/optional.h"

namespace blink {
class WebMediaConstraints;
}

namespace content {

CONTENT_EXPORT extern const int kMinScreenCastDimension;
CONTENT_EXPORT extern const int kMaxScreenCastDimension;
CONTENT_EXPORT extern const int kDefaultScreenCastWidth;
CONTENT_EXPORT extern const int kDefaultScreenCastHeight;

CONTENT_EXPORT extern const double kMaxScreenCastFrameRate;
CONTENT_EXPORT extern const double kDefaultScreenCastFrameRate;

// This function performs source, source-settings and track-settings selection
// for content video capture based on the given |constraints|.
// |stream_source| is used to determine the resolution-change policy.
// |device_id| allows specifying a device ID as the only valid device ID
// to the algorithm. If no device_id is specified, any device ID given in
// |constraints| will be considered as valid.
// To select the target width, height and frame rate, this function gives
// priority to ideal values provided in |constraints|. If no ideal values are
// given, maximum values from |constraints| are used. If no maximum values are
// given, the provided |default_width|, |default_height| and
// |default_frame_rate| are used. A |default_frame rate| of 0.0 means the
// default is the source's frame rate.
VideoCaptureSettings CONTENT_EXPORT
SelectSettingsVideoContentCapture(const blink::WebMediaConstraints& constraints,
                                  const std::string& stream_source,
                                  int default_width,
                                  int default_height,
                                  double default_frame_rate,
                                  const base::Optional<std::string>& device_id);

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_MEDIA_STREAM_CONSTRAINTS_UTIL_VIDEO_CONTENT_H_
