// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ThreadType_h
#define ThreadType_h

#include "public/platform/WebCommon.h"

namespace blink {

enum class ThreadType {
  kMainThread = 0,
  kUnspecifiedWorkerThread = 1,
  kCompositorThread = 2,
  kDedicatedWorkerThread = 3,
  kSharedWorkerThread = 4,
  kAnimationWorkletThread = 5,
  kServiceWorkerThread = 6,
  kAudioWorkletThread = 7,
  kFileThread = 8,
  kDatabaseThread = 9,
  kWebAudioThread = 10,
  kScriptStreamerThread = 11,
  kOfflineAudioRenderThread = 12,
  kFIFOClientThread = 13,
  kReverbConvolutionBackgroundThread = 14,
  kHRTFDatabaseLoaderThread = 15,
  kTestThread = 16,

  kCount = 17
};

BLINK_PLATFORM_EXPORT const char* GetNameForThreadType(ThreadType);

}  // namespace blink

#endif  // ThreadType_h
