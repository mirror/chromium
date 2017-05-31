// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_MEDIA_RENDERER_AUDIO_OUTPUT_STREAM_FACTORY_CONTEXT_IMPL_H_
#define CONTENT_BROWSER_RENDERER_HOST_MEDIA_RENDERER_AUDIO_OUTPUT_STREAM_FACTORY_CONTEXT_IMPL_H_

#include <memory>
#include <string>
#include <utility>

#include "base/containers/flat_map.h"
#include "base/memory/weak_ptr.h"
#include "content/browser/renderer_host/media/audio_output_authorization_handler.h"
#include "content/browser/renderer_host/media/render_frame_audio_output_stream_factory.h"
#include "content/browser/renderer_host/media/renderer_audio_output_stream_factory_context.h"
#include "content/common/media/renderer_audio_output_stream_factory.mojom.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace media {
class AudioManager;
class AudioSystem;
}  // namespace media

namespace content {

class MediaStreamManager;

// In addition to being a RendererAudioOutputStreamFactoryContext, this class
// also handles requests for mojom::RendererAudioOutputStreamFactory instances.
//
// Ownership diagram for stream IPC classes (excluding interfaces):
// RendererAudioOutputStreamFactoryContext
//                 ^
//                 | owns (at most one per render frame in the process).
//                 |
// RenderFrameAudioOutputStreamFactory
//                 ^
//                 | owns (one per stream for the frame).
//                 |
// media::MojoAudioOutputStreamProvider
//                 ^
//                 | owns (one).
//                 |
// media::MojoAudioOutputStream

class CONTENT_EXPORT RendererAudioOutputStreamFactoryContextImpl
    : public RendererAudioOutputStreamFactoryContext {
 public:
  // A handle for managing the lifetime of created factories.
  class CONTENT_EXPORT FactoryHandle {
   public:
    FactoryHandle();
    FactoryHandle(FactoryHandle&& other);
    FactoryHandle& operator=(FactoryHandle&& other);

    ~FactoryHandle();

   private:
    friend class RendererAudioOutputStreamFactoryContextImpl;

    struct ImplWithBinding;

    FactoryHandle(
        RendererAudioOutputStreamFactoryContextImpl* context,
        int frame_id,
        mojo::InterfaceRequest<mojom::RendererAudioOutputStreamFactory>);

    std::unique_ptr<ImplWithBinding, BrowserThread::DeleteOnIOThread> factory_;
    DISALLOW_COPY_AND_ASSIGN(FactoryHandle);
  };

  RendererAudioOutputStreamFactoryContextImpl(
      int render_process_id,
      media::AudioSystem* audio_system,
      media::AudioManager* audio_manager,
      MediaStreamManager* media_stream_manager,
      const std::string& salt);

  ~RendererAudioOutputStreamFactoryContextImpl() override;

  // Creates a factory and binds it to the request. The returned FactoryHandle
  // is used to manage the lifetime of the created factory. It should not
  // outlive the frame referred to by |frame_id|.
  FactoryHandle CreateFactory(
      int frame_id,
      mojo::InterfaceRequest<mojom::RendererAudioOutputStreamFactory> request);

  void CreateFactoryOnIO(
      int frame_host_id,
      mojo::InterfaceRequest<mojom::RendererAudioOutputStreamFactory> request);

  // RendererAudioOutputStreamFactoryContext implementation.
  int GetRenderProcessId() const override;

  std::string GetHMACForDeviceId(
      const url::Origin& origin,
      const std::string& raw_device_id) const override;

  void RequestDeviceAuthorization(
      int render_frame_id,
      int session_id,
      const std::string& device_id,
      const url::Origin& security_origin,
      AuthorizationCompletedCallback cb) const override;

  std::unique_ptr<media::AudioOutputDelegate> CreateDelegate(
      const std::string& unique_device_id,
      int render_frame_id,
      const media::AudioParameters& params,
      media::AudioOutputDelegate::EventHandler* handler) override;

  static bool UseMojoFactories();

 private:
  // Used for hashing the device_id.
  const std::string salt_;
  media::AudioSystem* const audio_system_;
  media::AudioManager* const audio_manager_;
  MediaStreamManager* const media_stream_manager_;
  const AudioOutputAuthorizationHandler authorization_handler_;
  const int render_process_id_;

  // All streams requires ids for logging, so we keep a count for that.
  int next_stream_id_ = 0;

  DISALLOW_COPY_AND_ASSIGN(RendererAudioOutputStreamFactoryContextImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_MEDIA_RENDERER_AUDIO_OUTPUT_STREAM_FACTORY_CONTEXT_IMPL_H_
