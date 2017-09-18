// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/workers/WorkerMediaPlayer.h"

#include "core/exported/WebAssociatedURLLoaderImpl.h"
#include "core/workers/WorkerGlobalScope.h"
#include "core/workers/WorkerMediaPlayerFactory.h"
#include "platform/loader/fetch/ResourceFetcher.h"
#include "public/platform/WebMediaPlayerFactory.h"
#include "public/web/WebFetchContext.h"

namespace blink {

namespace {
class WebFetchContextImpl
    : public GarbageCollectedFinalized<WebFetchContextImpl>,
      public WebFetchContext {
 public:
  WebFetchContextImpl(ExecutionContext* context)
      : execution_context_(context) {}
  ~WebFetchContextImpl() override = default;

  WebSecurityOrigin GetSecurityOrigin() override {
    return WebSecurityOrigin(GetFetchContext().GetSecurityOrigin());
  }
  std::unique_ptr<WebAssociatedURLLoader> CreateUrlLoader(
      const WebAssociatedURLLoaderOptions& options) override {
    return WTF::MakeUnique<WebAssociatedURLLoaderImpl>(execution_context_,
                                                       options);
  }

  DEFINE_INLINE_TRACE() { visitor->Trace(execution_context_); }

 private:
  FetchContext& GetFetchContext() {
    WorkerGlobalScope* global_scope = ToWorkerGlobalScope(execution_context_);
    DCHECK(global_scope);
    return global_scope->GetResourceFetcher()->Context();
  }

  WeakMember<ExecutionContext> execution_context_;
};
}  // namespace

WorkerMediaPlayer::WorkerMediaPlayer(ExecutionContext* context)
    : media_player_(new MediaPlayer(context, this, this)) {}

ExecutionContext* WorkerMediaPlayer::GetExecutionContext() const {
  return media_player_->GetExecutionContext();
}

DEFINE_TRACE(WorkerMediaPlayer) {
  visitor->Trace(media_player_);

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

  // TODO(alokp): Initialize the factory only once.
  factory->Initialize(new WebFetchContextImpl(global_scope));

  return factory->CreateMediaPlayer(source, client, nullptr, nullptr, "");
}

}  // namespace blink
