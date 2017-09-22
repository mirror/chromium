// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/workers/WorkerMediaPlayer.h"

#include "core/exported/WebAssociatedURLLoaderImpl.h"
#include "core/workers/WorkerGlobalScope.h"
#include "core/workers/WorkerMediaPlayerFactory.h"
#include "platform/loader/fetch/ResourceFetcher.h"
#include "public/platform/WebMediaPlayerFactory.h"

namespace blink {

WebFetchContextImpl::WebFetchContextImpl(ExecutionContext* context)
    : execution_context_(context) {}

WebFetchContextImpl::~WebFetchContextImpl() {}

WebSecurityOrigin WebFetchContextImpl::GetSecurityOrigin() {
  WorkerGlobalScope* global_scope = ToWorkerGlobalScope(execution_context_);
  return WebSecurityOrigin(global_scope->GetResourceFetcher()->Context().GetSecurityOrigin());
}

std::unique_ptr<WebAssociatedURLLoader> WebFetchContextImpl::CreateUrlLoader(
    const WebAssociatedURLLoaderOptions& options) {
  return WTF::MakeUnique<WebAssociatedURLLoaderImpl>(execution_context_,
                                                     options);
}

DEFINE_TRACE(WebFetchContextImpl) {
  visitor->Trace(execution_context_);
}

WorkerMediaPlayer::WorkerMediaPlayer(ExecutionContext* context)
    : media_player_(new MediaPlayer(context, this, this)) {}

ExecutionContext* WorkerMediaPlayer::GetExecutionContext() const {
  return media_player_->GetExecutionContext();
}

DEFINE_TRACE(WorkerMediaPlayer) {
  visitor->Trace(media_player_);
  visitor->Trace(fetch_context_);

  EventTargetWithInlineData::Trace(visitor);
}

std::unique_ptr<WebMediaPlayer> WorkerMediaPlayer::CreateWebMediaPlayer(
    const WebMediaPlayerSource& source,
    WebMediaPlayerClient* client) {
  WorkerGlobalScope* global_scope = ToWorkerGlobalScope(GetExecutionContext());
  if (!global_scope)
    return nullptr;

  WebMediaPlayerFactory* factory =
      WorkerMediaPlayerFactory::GetFrom(*global_scope->Clients());
  if (!factory)
    return nullptr;

  if (!fetch_context_) {
    fetch_context_ = new WebFetchContextImpl(global_scope);
    // TODO(alokp): Initialize the factory only once.
    factory->Initialize(fetch_context_);
  }

  return factory->CreateMediaPlayer(source, client, nullptr, nullptr, "");
}

}  // namespace blink
