// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/mojo/mock_media_route_provider.h"

namespace media_router {

MockMediaRouteProvider::MockMediaRouteProvider() {}

MockMediaRouteProvider::~MockMediaRouteProvider() {}

void MockMediaRouteProvider::RouteRequestSuccess(RouteCallback& cb) const {
  DCHECK(route_);
  std::move(cb).Run(route_, std::string(), RouteRequestResult::OK);
}

void MockMediaRouteProvider::RouteRequestTimeout(RouteCallback& cb) const {
  std::move(cb).Run(base::nullopt, std::string("error"),
                    RouteRequestResult::TIMED_OUT);
}

void MockMediaRouteProvider::TerminateRouteSuccess(
    TerminateRouteCallback& cb) const {
  std::move(cb).Run(std::string(), RouteRequestResult::OK);
}

void MockMediaRouteProvider::SendRouteMessageSuccess(
    SendRouteMessageCallback& cb) const {
  std::move(cb).Run(true);
}
void MockMediaRouteProvider::SendRouteBinaryMessageSuccess(
    SendRouteBinaryMessageCallback& cb) const {
  std::move(cb).Run(true);
}

void MockMediaRouteProvider::SearchSinksSuccess(SearchSinksCallback& cb) const {
  std::string sink_id = route_ ? route_->media_sink_id() : std::string();
  std::move(cb).Run(sink_id);
}

void MockMediaRouteProvider::CreateMediaRouteControllerSuccess(
    CreateMediaRouteControllerCallback& cb) const {
  std::move(cb).Run(true);
}

void MockMediaRouteProvider::SetRouteToReturn(const MediaRoute& route) {
  route_ = route;
}

}  // namespace media_router
