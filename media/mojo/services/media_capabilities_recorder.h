// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_MEDIA_CAPABILITIES_RECORDER_H_
#define MEDIA_MOJO_SERVICES_MEDIA_CAPABILITIES_RECORDER_H_

#include <stdint.h>
#include <string>

#include "base/time/time.h"
#include "media/base/video_codecs.h"
#include "media/mojo/interfaces/media_capabilities_recorder.mojom.h"
#include "media/mojo/services/media_mojo_export.h"
#include "services/service_manager/public/cpp/bind_source_info.h"

namespace media {

// See mojom::MediaCapabilitiesRecorder for documentation.
class MEDIA_MOJO_EXPORT MediaCapabilitiesRecorder
    : public mojom::MediaCapabilitiesRecorder {
 public:
  MediaCapabilitiesRecorder();
  ~MediaCapabilitiesRecorder() override;

  static void Create(mojom::MediaCapabilitiesRecorderRequest request);

  // mojom::MediaCapabilitiesRecorder implementation:
  void StartNewRecord(VideoCodecProfile profile,
                      const gfx::Size& natural_size,
                      int frames_per_sec) override;

  // FIXME - THESE SHOULD BE UINT32_T
  void UpdateRecord(int frames_decoded, int frames_dropped) override;
  void FinalizeRecord() override;

 private:
  VideoCodecProfile profile_ = VIDEO_CODEC_PROFILE_UNKNOWN;
  gfx::Size natural_size_;
  int frames_per_sec_ = 0;
  int frames_decoded_ = 0;
  int frames_dropped_ = 0;

  DISALLOW_COPY_AND_ASSIGN(MediaCapabilitiesRecorder);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MEDIA_CAPABILITIES_RECORDER_H_