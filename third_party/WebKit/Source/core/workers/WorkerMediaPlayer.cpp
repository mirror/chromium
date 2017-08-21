// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/workers/WorkerMediaPlayer.h"

#include "core/workers/WorkerGlobalScope.h"
#include "core/workers/WorkerMediaPlayerFactory.h"
#include "public/platform/WebMediaPlayerFactory.h"

namespace blink {

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

  factory->SetSecurityOrigin(
      WebSecurityOrigin(global_scope->GetSecurityOrigin()));
  return factory->CreateMediaPlayer(source, client, nullptr, nullptr, "");
}

}  // namespace blink
