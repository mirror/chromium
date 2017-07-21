// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_MOJO_AUDIO_INPUT_IPC_H_
#define CONTENT_RENDERER_MEDIA_MOJO_AUDIO_INPUT_IPC_H_

#include <string>

#include "base/callback_helpers.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "content/common/content_export.h"
#include "content/common/media/renderer_audio_input_stream_factory.mojom.h"
#include "media/audio/audio_input_ipc.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace content {

// MojoAudioInputIPC is a renderer-side class for handling creation,
// initialization and control of an input stream. May only be used on a single
// thread.
class CONTENT_EXPORT MojoAudioInputIPC
    : public media::AudioInputIPC,
      public media::mojom::AudioInputStreamClient {
 public:
  using FactoryAccessorCB =
      base::RepeatingCallback<mojom::RendererAudioInputStreamFactory*()>;

  // |factory_accessor| is required to provide a
  // RendererAudioInputStreamFactory* if IPC is possible.
  explicit MojoAudioInputIPC(FactoryAccessorCB factory_accessor);
  ~MojoAudioInputIPC() override;

  // AudioInputIPC implementation
  void CreateStream(media::AudioInputIPCDelegate* delegate,
                    int session_id,
                    const media::AudioParameters& params,
                    bool automatic_gain_control,
                    uint32_t total_segments) override;
  void RecordStream() override;
  void SetVolume(double volume) override;
  void CloseStream() override;

 private:
  void StreamCreated(mojo::ScopedSharedBufferHandle shared_memory,
                     mojo::ScopedHandle socket,
                     uint32_t length,
                     uint32_t segment_count,
                     bool initially_muted);

  void OnMuted(bool is_muted) override;

  FactoryAccessorCB factory_accessor_;

  THREAD_CHECKER(thread_checker_);

  media::mojom::AudioInputStreamPtr stream_;
  mojo::Binding<AudioInputStreamClient> client_binding_;
  media::AudioInputIPCDelegate* delegate_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(MojoAudioInputIPC);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_MOJO_AUDIO_INPUT_IPC_H_
