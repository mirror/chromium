// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/mojo/media_router_desktop_impl.h"

#include "chrome/browser/media/router/event_page_request_manager.h"

namespace media_router {

MediaRouterDesktopImpl::~MediaRouterDesktopImpl() = default;

void MediaRouterDesktopImpl::DoCreateRoute(
    const MediaSource::Id& source_id,
    const MediaSink::Id& sink_id,
    const url::Origin& origin,
    int tab_id,
    std::vector<MediaRouteResponseCallback> callbacks,
    base::TimeDelta timeout,
    bool incognito) {
  if (event_page_request_manager_->mojo_connections_ready()) {
    MediaRouterMojoImpl::DoCreateRoute(source_id, sink_id, origin, tab_id,
                                       std::move(callbacks), timeout,
                                       incognito);
    return;
  }
  event_page_request_manager_->RunOrDefer(
      base::BindOnce(&MediaRouterDesktopImpl::DoCreateRoute,
                     weak_factory_.GetWeakPtr(), source_id, sink_id, origin,
                     tab_id, std::move(callbacks), timeout, incognito),
      MediaRouteProviderWakeReason::CREATE_ROUTE);
}

MediaRouterDesktopImpl::MediaRouterDesktopImpl(content::BrowserContext* context)
    : MediaRouterMojoImpl(context), weak_factory_(this) {}

}  // namespace media_router
