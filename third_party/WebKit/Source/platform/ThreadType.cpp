// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/platform/ThreadType.h"

#include "base/logging.h"

namespace blink {

const char* GetNameForThreadType(ThreadType thread_type) {
  switch (thread_type) {
    case ThreadType::kMainThread:
      return "Main thread";
    case ThreadType::kUnspecifiedWorkerThread:
      return "unspecified worker thread";
    case ThreadType::kCompositorThread:
      return "Compositor thread";
    case ThreadType::kDedicatedWorkerThread:
      return "DedicatedWorker thread";
    case ThreadType::kSharedWorkerThread:
      return "SharedWorker thread";
    case ThreadType::kAnimationWorkletThread:
      return "AnimationWorklet thread";
    case ThreadType::kServiceWorkerThread:
      return "ServiceWorker thread";
    case ThreadType::kAudioWorkletThread:
      return "AudioWorklet thread";
    case ThreadType::kFileThread:
      return "File thread";
    case ThreadType::kDatabaseThread:
      return "Database thread";
    case ThreadType::kWebAudioThread:
      return "WebAudio thread";
    case ThreadType::kScriptStreamerThread:
      return "ScriptStreamer thread";
    case ThreadType::kOfflineAudioRenderThread:
      return "OfflineAudioRender thread";
    case ThreadType::kFIFOClientThread:
      return "FIFOClient thread";
    case ThreadType::kReverbConvolutionBackgroundThread:
      return "Reverb convolution background thread";
    case ThreadType::kHRTFDatabaseLoaderThread:
      return "HRTF database loader thread";
    case ThreadType::kTestThread:
      return "test thread";
    case ThreadType::kCount:
      NOTREACHED();
      return nullptr;
  }
}

}  // namespace blink
