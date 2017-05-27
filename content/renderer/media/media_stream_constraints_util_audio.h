// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_MEDIA_STREAM_CONSTRAINTS_UTIL_AUDIO_H_
#define CONTENT_RENDERER_MEDIA_MEDIA_STREAM_CONSTRAINTS_UTIL_AUDIO_H_

#include <string>
#include <vector>

#include "content/common/content_export.h"
#include "content/common/media/media_devices.mojom.h"
#include "content/renderer/media/media_stream_constraints_util.h"

namespace blink {
class WebMediaConstraints;
}

namespace content {

using AudioDeviceCaptureCapabilities =
    std::vector<::mojom::AudioInputDeviceCapabilitiesPtr>;

AudioCaptureSettings CONTENT_EXPORT
SelectSettingsAudioCapture(const AudioDeviceCaptureCapabilities& capabilities,
                           const blink::WebMediaConstraints& constraints);

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_MEDIA_STREAM_CONSTRAINTS_UTIL_AUDIO_H_
