// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/mojo/media_router_mojo_test.h"

#include <utility>

#include "base/run_loop.h"

namespace media_router {
namespace {
const char kInstanceId[] = "instance123";
}  // namespace

MockMediaRouteProvider::MockMediaRouteProvider() {}

MockMediaRouteProvider::~MockMediaRouteProvider() {}

void MockMediaRouteProvider::RouteRequestSuccess(RouteCallback& cb) const {
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
  route_ = base::make_optional(route);
}

MockEventPageTracker::MockEventPageTracker() {}

MockEventPageTracker::~MockEventPageTracker() {}

MockMediaController::MockMediaController() : binding_(this) {}

MockMediaController::~MockMediaController() {}

RegisterMediaRouteProviderHandler::RegisterMediaRouteProviderHandler() {}

RegisterMediaRouteProviderHandler::~RegisterMediaRouteProviderHandler() {}

void MockMediaController::Bind(mojom::MediaControllerRequest request) {
  binding_.Bind(std::move(request));
}

mojom::MediaControllerPtr MockMediaController::BindInterfacePtr() {
  mojom::MediaControllerPtr controller;
  binding_.Bind(mojo::MakeRequest(&controller));
  return controller;
}

void MockMediaController::CloseBinding() {
  binding_.Close();
}

MockMediaRouteController::MockMediaRouteController(
    const MediaRoute::Id& route_id,
    mojom::MediaControllerPtr mojo_media_controller,
    MediaRouter* media_router)
    : MediaRouteController(route_id,
                           std::move(mojo_media_controller),
                           media_router) {}

MockMediaRouteController::~MockMediaRouteController() {}

MockMediaRouteControllerObserver::MockMediaRouteControllerObserver(
    scoped_refptr<MediaRouteController> controller)
    : MediaRouteController::Observer(controller) {}

MockMediaRouteControllerObserver::~MockMediaRouteControllerObserver() {}

MediaRouterMojoTest::MediaRouterMojoTest() {}

MediaRouterMojoTest::~MediaRouterMojoTest() {}

void MediaRouterMojoTest::ConnectProviderManagerService() {
  // Bind the |media_route_provider| interface to |media_route_provider_|.
  auto request = mojo::MakeRequest(&media_router_proxy_);
  mock_media_router_->BindToMojoRequest(std::move(request),
                                        base::OnceClosure());

  // Bind the Mojo MediaRouter interface used by |mock_media_router_| to
  // |mock_media_route_provider_service_|.
  mojom::MediaRouteProviderPtr mojo_media_router;
  binding_ = base::MakeUnique<mojo::Binding<mojom::MediaRouteProvider>>(
      &mock_media_route_provider_, mojo::MakeRequest(&mojo_media_router));
  EXPECT_CALL(provide_handler_, InvokeInternal(kInstanceId, testing::_));
  media_router_proxy_->RegisterMediaRouteProvider(
      std::move(mojo_media_router),
      base::Bind(&RegisterMediaRouteProviderHandler::Invoke,
                 base::Unretained(&provide_handler_)));
}

void MediaRouterMojoTest::SetUp() {
  mock_media_router_.reset(new MediaRouterMojoImpl(&profile_));
  mock_media_router_->Initialize();
  mock_media_router_->set_instance_id_for_test(kInstanceId);
  ConnectProviderManagerService();
  base::RunLoop().RunUntilIdle();
}

void MediaRouterMojoTest::TearDown() {
  mock_media_router_->Shutdown();
}

void MediaRouterMojoTest::ProcessEventLoop() {
  base::RunLoop().RunUntilIdle();
}

}  // namespace media_router
