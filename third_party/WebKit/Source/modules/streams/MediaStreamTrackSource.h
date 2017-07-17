// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MediaStreamTrackSource_h
#define MediaStreamTrackSource_h

#include "bindings/core/v8/ScriptPromise.h"
#include "bindings/core/v8/ScriptValue.h"
#include "core/streams/UnderlyingSourceBase.h"
#include "modules/ModulesExport.h"
#include "public/platform/WebVideoFrameReaderClient.h"

namespace blink {

class MediaStreamTrack;
class WebVideoFrameReader;

class MODULES_EXPORT MediaStreamTrackSource final
    : public UnderlyingSourceBase,
      public WebVideoFrameReaderClient {
  WTF_MAKE_NONCOPYABLE(MediaStreamTrackSource);
  USING_GARBAGE_COLLECTED_MIXIN(MediaStreamTrackSource);

 public:
  MediaStreamTrackSource(ScriptState*, MediaStreamTrack&);

  // UnderlyingSourceBase
  ScriptPromise Start(ScriptState*) override;
  ScriptPromise pull(ScriptState*) override;
  ScriptPromise Cancel(ScriptState*, ScriptValue reason) override;
  bool HasPendingActivity() const override;
  void ContextDestroyed(ExecutionContext*) override;

  // WebVideoFrameReaderClient impl
  void OnVideoFrame(sk_sp<SkImage>) override;
  void OnError(const WebString& message) override;

  DECLARE_VIRTUAL_TRACE();

 private:
  RefPtr<ScriptState> script_state_;
  Member<MediaStreamTrack> stream_track_;

  std::unique_ptr<WebVideoFrameReader> video_frame_reader_;
};

}  // namespace blink

#endif  // MediaStreamTrackSource_h
