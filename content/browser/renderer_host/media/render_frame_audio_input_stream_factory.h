// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_MEDIA_RENDER_FRAME_AUDIO_INPUT_STREAM_FACTORY_H_
#define CONTENT_BROWSER_RENDERER_HOST_MEDIA_RENDER_FRAME_AUDIO_INPUT_STREAM_FACTORY_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/containers/flat_set.h"
#include "content/browser/renderer_host/media/audio_input_device_manager.h"
#include "content/common/content_export.h"
#include "content/common/media/renderer_audio_input_stream_factory.mojom.h"
#include "content/public/browser/browser_thread.h"
#include "media/audio/audio_input_delegate.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace media {
class AudioParameters;
}  // namespace media

namespace content {

// Handles a RendererAudioInputStreamFactory request for a render frame host,
// using the provided RendererAudioInputStreamFactoryContext. This class may
// be constructed on any thread, but must be used on the IO thread after that.
class CONTENT_EXPORT RenderFrameAudioInputStreamFactory
    : public mojom::RendererAudioInputStreamFactory {
 public:
  using CreateDelegateCallback =
      base::RepeatingCallback<std::unique_ptr<media::AudioInputDelegate>(
          AudioInputDeviceManager::KeyboardMicRegistration
              keyboard_mic_registration,
          uint32_t shared_memory_count,
          int stream_id,
          int session_id,
          bool automatic_gain_control,
          const media::AudioParameters& parameters,
          media::AudioInputDelegate::EventHandler* event_handler)>;

  RenderFrameAudioInputStreamFactory(
      CreateDelegateCallback create_delegate_callback,
      content::AudioInputDeviceManager* audio_input_device_manager);

  ~RenderFrameAudioInputStreamFactory() override;

 private:
  using InputStreamSet =
      base::flat_set<std::unique_ptr<media::mojom::AudioInputStream>>;

  // mojom::RendererAudioInputStreamFactory implementation.
  void CreateStream(media::mojom::AudioInputStreamRequest stream_request,
                    int64_t session_id,
                    const media::AudioParameters& audio_params,
                    bool automatic_gain_control,
                    uint32_t shared_memory_count,
                    media::mojom::AudioInputStreamClientPtr client) override;

  void DoCreateStream(media::mojom::AudioInputStreamRequest stream_request,
                      int64_t session_id,
                      const media::AudioParameters& audio_params,
                      bool automatic_gain_control,
                      uint32_t shared_memory_count,
                      media::mojom::AudioInputStreamClientPtr client,
                      AudioInputDeviceManager::KeyboardMicRegistration
                          keyboard_mic_registration);

  void RemoveStream(media::mojom::AudioInputStream* input_stream);

  const CreateDelegateCallback create_delegate_callback_;
#if defined(OS_CHROMEOS)
  content::AudioInputDeviceManager* const audio_input_device_manager_;
#endif

  InputStreamSet streams_;
  int next_stream_id_ = 0;

  base::WeakPtrFactory<RenderFrameAudioInputStreamFactory> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(RenderFrameAudioInputStreamFactory);
};

// This class is a convenient bundle of factory and binding.
class CONTENT_EXPORT RenderFrameAudioInputStreamFactoryHandle {
 public:
  static std::unique_ptr<RenderFrameAudioInputStreamFactoryHandle,
                         BrowserThread::DeleteOnIOThread>
  CreateFactory(RenderFrameAudioInputStreamFactory::CreateDelegateCallback
                    create_delegate_callback,
                content::AudioInputDeviceManager* audio_input_device_manager,
                mojom::RendererAudioInputStreamFactoryRequest request);

  ~RenderFrameAudioInputStreamFactoryHandle();

 private:
  RenderFrameAudioInputStreamFactoryHandle(
      RenderFrameAudioInputStreamFactory::CreateDelegateCallback
          create_delegate_callback,
      content::AudioInputDeviceManager* audio_input_device_manager);

  void Init(mojom::RendererAudioInputStreamFactoryRequest request);

  RenderFrameAudioInputStreamFactory impl_;
  mojo::Binding<mojom::RendererAudioInputStreamFactory> binding_;

  DISALLOW_COPY_AND_ASSIGN(RenderFrameAudioInputStreamFactoryHandle);
};

using UniqueAudioInputStreamFactoryPtr =
    std::unique_ptr<RenderFrameAudioInputStreamFactoryHandle,
                    BrowserThread::DeleteOnIOThread>;

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_MEDIA_RENDER_FRAME_AUDIO_INPUT_STREAM_FACTORY_H_
