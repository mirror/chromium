// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mirror/mirror_service_threads.h"

#include "base/logging.h"
#include "base/single_thread_task_runner.h"

MirrorServiceThreads::MirrorServiceThreads()
    : audio_encode_thread_("CastAudioEncodeThread"),
      video_encode_thread_("CastVideoEncodeThread") {
  audio_encode_thread_.Start();
  video_encode_thread_.Start();
}

scoped_refptr<base::SingleThreadTaskRunner>
MirrorServiceThreads::GetAudioEncodeTaskRunner() {
  return audio_encode_thread_.task_runner();
}

scoped_refptr<base::SingleThreadTaskRunner>
MirrorServiceThreads::GetVideoEncodeTaskRunner() {
  return video_encode_thread_.task_runner();
}
