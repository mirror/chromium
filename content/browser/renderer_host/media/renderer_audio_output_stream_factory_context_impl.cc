// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/media/renderer_audio_output_stream_factory_context_impl.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "content/browser/media/capture/audio_mirroring_manager.h"
#include "content/browser/media/media_internals.h"
#include "content/browser/renderer_host/media/audio_output_delegate_impl.h"
#include "content/browser/renderer_host/media/media_stream_manager.h"
#include "content/browser/renderer_host/media/render_frame_audio_output_stream_factory.h"
#include "content/common/media/renderer_audio_output_stream_factory.mojom.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "media/audio/audio_system.h"

namespace content {

using FactoryToken = RendererAudioOutputStreamFactoryContextImpl::FactoryToken;

FactoryToken::~FactoryToken() {
  FreeFactory();
}

FactoryToken::FactoryToken(RendererAudioOutputStreamFactoryContextImpl* context,
                           int frame_id)
    : context_(context), frame_id_(frame_id) {}

FactoryToken::FactoryToken(FactoryToken&& other)
    : context_(other.context_), frame_id_(other.frame_id_) {
  other.context_ = nullptr;
}

FactoryToken& FactoryToken::operator=(FactoryToken&& other) {
  FreeFactory();
  context_ = other.context_;
  frame_id_ = other.frame_id_;
  other.context_ = nullptr;
  return *this;
}

void FactoryToken::FreeFactory() {
  if (context_) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(
            &RendererAudioOutputStreamFactoryContextImpl::RemoveMapEntry,
            base::Unretained(context_), frame_id_));
  }
}

struct RendererAudioOutputStreamFactoryContextImpl::ImplWithBinding {
  ImplWithBinding(int frame_id,
                  RendererAudioOutputStreamFactoryContextImpl* context,
                  mojom::RendererAudioOutputStreamFactoryRequest request)
      : impl_(frame_id, context), binding_(&impl_, std::move(request)) {}

  RenderFrameAudioOutputStreamFactory impl_;
  mojo::Binding<mojom::RendererAudioOutputStreamFactory> binding_;
};

RendererAudioOutputStreamFactoryContextImpl::
    RendererAudioOutputStreamFactoryContextImpl(
        int render_process_id,
        media::AudioSystem* audio_system,
        media::AudioManager* audio_manager,
        MediaStreamManager* media_stream_manager,
        const std::string& salt)
    : salt_(salt),
      audio_system_(audio_system),
      audio_manager_(audio_manager),
      media_stream_manager_(media_stream_manager),
      authorization_handler_(audio_system_,
                             media_stream_manager_,
                             render_process_id,
                             salt_),
      render_process_id_(render_process_id) {}

RendererAudioOutputStreamFactoryContextImpl::
    ~RendererAudioOutputStreamFactoryContextImpl() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(factories_.empty());
}

FactoryToken RendererAudioOutputStreamFactoryContextImpl::CreateFactory(
    int frame_id,
    mojo::InterfaceRequest<mojom::RendererAudioOutputStreamFactory> request) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // Unretained is safe since this object should be posted to the IO thread
  // before destruction, and since we're accessing it from the UI thread, this
  // hasn't been done yet.
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(
          &RendererAudioOutputStreamFactoryContextImpl::CreateFactoryOnIO,
          base::Unretained(this), frame_id, std::move(request)));

  return FactoryToken(this, frame_id);
}

void RendererAudioOutputStreamFactoryContextImpl::CreateFactoryOnIO(
    int frame_id,
    mojo::InterfaceRequest<mojom::RendererAudioOutputStreamFactory> request) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!request.is_pending()) {
    // Request was canceled before getting here.
    RemoveMapEntry(frame_id);
    return;
  }

  std::pair<FactoryBindingMap::iterator, bool> emplace_result =
      factories_.emplace(frame_id, base::MakeUnique<ImplWithBinding>(
                                       frame_id, this, std::move(request)));

  DCHECK(emplace_result.second);

  mojo::Binding<mojom::RendererAudioOutputStreamFactory>& binding =
      emplace_result.first->second->binding_;

  // Unretained is safe because |this| owns |binding|.
  binding.set_connection_error_handler(base::BindRepeating(
      &RendererAudioOutputStreamFactoryContextImpl::RemoveMapEntry,
      base::Unretained(this), frame_id));
}

int RendererAudioOutputStreamFactoryContextImpl::GetRenderProcessId() const {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  return render_process_id_;
}

std::string RendererAudioOutputStreamFactoryContextImpl::GetHMACForDeviceId(
    const url::Origin& origin,
    const std::string& raw_device_id) const {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  return MediaStreamManager::GetHMACForMediaDeviceID(salt_, origin,
                                                     raw_device_id);
}

void RendererAudioOutputStreamFactoryContextImpl::RequestDeviceAuthorization(
    int render_frame_id,
    int session_id,
    const std::string& device_id,
    const url::Origin& security_origin,
    AuthorizationCompletedCallback cb) const {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  authorization_handler_.RequestDeviceAuthorization(
      render_frame_id, session_id, device_id, security_origin, std::move(cb));
}

std::unique_ptr<media::AudioOutputDelegate>
RendererAudioOutputStreamFactoryContextImpl::CreateDelegate(
    const std::string& unique_device_id,
    int render_frame_id,
    const media::AudioParameters& params,
    media::AudioOutputDelegate::EventHandler* handler) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  int stream_id = next_stream_id_++;
  MediaObserver* const media_observer =
      GetContentClient()->browser()->GetMediaObserver();

  MediaInternals* const media_internals = MediaInternals::GetInstance();
  std::unique_ptr<media::AudioLog> audio_log = media_internals->CreateAudioLog(
      media::AudioLogFactory::AUDIO_OUTPUT_CONTROLLER);
  media_internals->SetWebContentsTitleForAudioLogEntry(
      stream_id, render_process_id_, render_frame_id, audio_log.get());

  return AudioOutputDelegateImpl::Create(
      handler, audio_manager_, std::move(audio_log),
      AudioMirroringManager::GetInstance(), media_observer, stream_id,
      render_frame_id, render_process_id_, params, unique_device_id);
}

// static
bool RendererAudioOutputStreamFactoryContextImpl::UseMojoFactories() {
  // TODO(maxmorin): Introduce a feature for this.
  return false;
}

void RendererAudioOutputStreamFactoryContextImpl::RemoveMapEntry(int frame_id) {
  factories_.erase(frame_id);
}

}  // namespace content
