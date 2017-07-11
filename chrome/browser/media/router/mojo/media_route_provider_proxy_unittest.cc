// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/mojo/media_route_provider_proxy.h"

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/media/router/event_page_request_manager.h"
#include "chrome/browser/media/router/event_page_request_manager_factory.h"
#include "chrome/browser/media/router/mojo/media_router_mojo_test.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::Invoke;

namespace media_router {

namespace {

class MockEventPageRequestManager : public EventPageRequestManager {
public:
  static std::unique_ptr<KeyedService> Create(
    content::BrowserContext* context) {
    return base::MakeUnique<MockEventPageRequestManager>(context);
  }

  explicit MockEventPageRequestManager(content::BrowserContext* context)
    : EventPageRequestManager(context) {}
  ~MockEventPageRequestManager() = default;

  MOCK_METHOD1(SetExtensionId, void(const std::string& extension_id));
  void RunOrDefer(base::OnceClosure request,
    MediaRouteProviderWakeReason wake_reason) override {
    RunOrDeferInternal(request, wake_reason);
  }
  MOCK_METHOD2(RunOrDeferInternal,
    void(base::OnceClosure& request,
      MediaRouteProviderWakeReason wake_reason));
  MOCK_METHOD0(OnMojoConnectionsReady, void());
  MOCK_METHOD0(OnMojoConnectionError, void());

private:
  DISALLOW_COPY_AND_ASSIGN(MockEventPageRequestManager);
};

constexpr char kMediaSourceId[] = "media_source_1";
constexpr char kSinkId[] = "sink_1";
constexpr char kPresentationId[] = "presentation_1";

}  // namespace

class MediaRouteProviderProxyTest : public ::testing::Test {
public:
  MediaRouteProviderProxyTest() = default;
  ~MediaRouteProviderProxyTest() override = default;

protected:
  void SetUp() override {
    profile_ = base::MakeUnique<TestingProfile>();
    provider_proxy_ = base::MakeUnique<MediaRouteProviderProxy>(profile_, media_router_);
    // Set up a mock ProcessManager instance.
    EventPageRequestManagerFactory::GetInstance()->SetTestingFactory(
      profile_.get(), &MockEventPageRequestManager::Create);
    request_manager_ = static_cast<MockEventPageRequestManager*>(
      EventPageRequestManagerFactory::GetApiForBrowserContext(profile_.get()));

    ON_CALL(*request_manager_, RunOrDeferInternal(_, _))
      .WillByDefault(Invoke([](base::OnceClosure& request,
        MediaRouteProviderWakeReason wake_reason) {
      std::move(request).Run();
    }));

    RegisterMockMediaRouteProvider();
  }

  void TearDown() override {
    profile_.reset();
  }

  std::unique_ptr<MediaRouterMojoImpl> media_router_;
  std::unique_ptr<MediaRouteProviderProxy> provider_proxy_;
  MockEventPageRequestManager* request_manager_ = nullptr;
  MockMediaRouteProvider mock_provider_;

  const std::string media_source_id_ = "media_source_1";
  const MediaSource media_source = MediaSource(media_source_id_);
  const std::string sink_id_ = "sink_1";
  const std::string presentation_id_ = "presentation_1";
  const url::Origin origin_ = url::Origin(GURL(""));
  const int tab_id_ = 3;
  const base::TimeDelta timeout_ = base::TimeDelta();
  const std::string route_id_ = "route_1";
  const MediaRoute route_ = MediaRoute(route_id_, media_source_, sink_id_,
    "", false, "", false);

private:
  void RegisterMockMediaRouteProvider() {
    mojom::MediaRouteProviderPtr provider_ptr;
    binding_ = base::MakeUnique<mojo::Binding<mojom::MediaRouteProvider>>(
      &mock_provider_, mojo::MakeRequest(&provider_ptr));
    auto callback = base::BindOnce([](const std::string& instance_id, mojom::MediaRouteProviderConfigPtr config) {});
    provider_proxy_->RegisterMediaRouteProvider(std::move(provider_ptr), std::move(callback));
  }

  std::unique_ptr<mojo::Binding<mojom::MediaRouteProvider>> binding_;
  std::unique_ptr<TestingProfile> profile_;
  content::TestBrowserThreadBundle thread_bundle_;


  DISALLOW_COPY_AND_ASSIGN(MediaRouteProviderProxyTest);
};

TEST_F(MediaRouterMojoImplTest, CreateRoute) {
  MediaSource media_source(kSource);
  MediaRoute expected_route(kRouteId, media_source, kSinkId, "", false, "",
                            false);
  mock_provider_->SetRouteToReturn(expected_route);

  EXPECT_CALL(
      *mock_provider_,
      CreateRouteInternal(kSource, kSinkId, _, url::Origin(GURL(kOrigin)),
                          kInvalidTabId, base::TimeDelta::FromMilliseconds(kTimeoutMillis), is_incognito, _))
      .WillOnce(Invoke(mock_provider_, &MockMediaRouteProvider::CreateRouteSuccess));

  RouteResponseCallbackHandler handler;
  EXPECT_CALL(handler, DoInvoke(Pointee(Equals(expected_route)), Not(""), "",
                                RouteRequestResult::OK));
  std::vector<MediaRouteResponseCallback> route_response_callbacks;
  route_response_callbacks.push_back(base::BindOnce(
      &RouteResponseCallbackHandler::Invoke, base::Unretained(&handler)));
  provider_proxy_->CreateRoute(kSource, kSinkId, kPresentationId,
                               url::Origin(GURL(kOrigin)), kTabId,
                        base::TimeDelta::FromMilliseconds(kTimeoutMillis), is_incognito,
                        std::move(route_response_callbacks),
                        false);
}

// TEST_F(MediaRouteProviderProxyTest, CreateRoute) {
//   EXPECT_CALL(*mock_provider_, CreateRoute(media_source_id_, sink_id_, presentation_id_, origin_, tab_id_, timeout_, false, _))
//     .WithArg<7>([this](mojom::MediaRouteProvider::CreateRouteCallback& callback) {
//       std::move(callback).Run(base::make_optional(route_),
//         base::make_optional(std::string()),
//         RouteRequestResult::OK);
//     });
//   // TODO: use RouteResponseCallbackHandler from mojoimplunittest
//   provider_proxy_->CreateRoute(media_source_id_, sink_id_, presentation_id_, origin_, tab_id_, timeout_, false, );
// }

// TODO: have a test case in which all the MRP methods get called, but do not
// invoke EXPECT_CALL because request manager is not ready

/*
TEST_F(MediaRouteProviderProxyTest, JoinRoute) {
  MediaSource media_source(kSource);
  MediaRoute expected_route(kRouteId, media_source, kSinkId, "", false, "",
    false);

  MediaRoute route = CreateMediaRoute();
  // Make sure the MR has received an update with the route, so it knows there
  // is a route to join.
  std::vector<MediaRoute> routes;
  routes.push_back(route);
  router()->OnRoutesUpdated(routes, std::string(), std::vector<std::string>());
  EXPECT_TRUE(router()->HasJoinableRoute());

  // Use a lambda function as an invocation target here to work around
  // a limitation with GMock::Invoke that prevents it from using move-only types
  // in runnable parameter lists.
  EXPECT_CALL(
    mock_media_route_provider_,
    JoinRouteInternal(
      kSource, kPresentationId, url::Origin(GURL(kOrigin)), kInvalidTabId,
      base::TimeDelta::FromMilliseconds(kTimeoutMillis), _, _))
    .WillOnce(
      Invoke([&route](const std::string& source,
        const std::string& presentation_id,
        const url::Origin& origin, int tab_id,
        base::TimeDelta timeout, bool incognito,
        mojom::MediaRouteProvider::JoinRouteCallback& cb) {
    std::move(cb).Run(route, std::string(), RouteRequestResult::OK);
  }));

  RouteResponseCallbackHandler handler;
  base::RunLoop run_loop;
  EXPECT_CALL(handler, DoInvoke(Pointee(Equals(expected_route)), Not(""), "",
    RouteRequestResult::OK))
    .WillOnce(InvokeWithoutArgs([&run_loop]() { run_loop.Quit(); }));
  std::vector<MediaRouteResponseCallback> route_response_callbacks;
  route_response_callbacks.push_back(base::Bind(
    &RouteResponseCallbackHandler::Invoke, base::Unretained(&handler)));
  provider_proxy_->JoinRoute(kSource, kPresentationId, url::Origin(GURL(kOrigin)),
    tab_id_,
    base::TimeDelta::FromMilliseconds(kTimeoutMillis), false,
    std::move(route_response_callbacks));
}
*/
