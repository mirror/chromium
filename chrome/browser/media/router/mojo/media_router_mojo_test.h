// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_MOJO_MEDIA_ROUTER_MOJO_TEST_H_
#define CHROME_BROWSER_MEDIA_ROUTER_MOJO_MEDIA_ROUTER_MOJO_TEST_H_

#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "chrome/browser/media/router/event_page_request_manager.h"
#include "chrome/browser/media/router/mock_media_router.h"
#include "chrome/browser/media/router/mojo/media_router_mojo_impl.h"
#include "chrome/browser/media/router/mojo/mock_media_route_provider.h"
#include "chrome/browser/media/router/test_helper.h"
#include "chrome/common/media_router/mojo/media_router.mojom.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "extensions/browser/event_page_tracker.h"
#include "extensions/common/extension.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media_router {

class MediaRouterMojoImpl;

class MockEventPageTracker : public extensions::EventPageTracker {
 public:
  MockEventPageTracker();
  ~MockEventPageTracker();

  MOCK_METHOD1(IsEventPageSuspended, bool(const std::string& extension_id));
  MOCK_METHOD2(WakeEventPage,
               bool(const std::string& extension_id,
                    const base::Callback<void(bool)>& callback));
};

class MockEventPageRequestManager : public EventPageRequestManager {
 public:
  static std::unique_ptr<KeyedService> Create(content::BrowserContext* context);

  explicit MockEventPageRequestManager(content::BrowserContext* context);
  ~MockEventPageRequestManager();

  MOCK_METHOD1(SetExtensionId, void(const std::string& extension_id));
  void RunOrDefer(base::OnceClosure request,
                  MediaRouteProviderWakeReason wake_reason) override;
  MOCK_METHOD2(RunOrDeferInternal,
               void(base::OnceClosure& request,
                    MediaRouteProviderWakeReason wake_reason));
  MOCK_METHOD0(OnMojoConnectionsReady, void());
  MOCK_METHOD0(OnMojoConnectionError, void());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockEventPageRequestManager);
};

class MockMediaController : public mojom::MediaController,
                            mojom::HangoutsMediaRouteController {
 public:
  MockMediaController();
  ~MockMediaController();

  void Bind(mojom::MediaControllerRequest request);
  mojom::MediaControllerPtr BindInterfacePtr();
  void CloseBinding();

  MOCK_METHOD0(Play, void());
  MOCK_METHOD0(Pause, void());
  MOCK_METHOD1(SetMute, void(bool mute));
  MOCK_METHOD1(SetVolume, void(float volume));
  MOCK_METHOD1(Seek, void(base::TimeDelta time));
  void ConnectHangoutsMediaRouteController(
      mojom::HangoutsMediaRouteControllerRequest controller_request) override {
    hangouts_binding_.Bind(std::move(controller_request));
    ConnectHangoutsMediaRouteController();
  };
  MOCK_METHOD0(ConnectHangoutsMediaRouteController, void());
  MOCK_METHOD1(SetLocalPresent, void(bool local_present));

 private:
  mojo::Binding<mojom::MediaController> binding_;
  mojo::Binding<mojom::HangoutsMediaRouteController> hangouts_binding_;
};

class MockMediaRouteController : public MediaRouteController {
 public:
  MockMediaRouteController(const MediaRoute::Id& route_id,
                           content::BrowserContext* context);
  MOCK_METHOD0(Play, void());
  MOCK_METHOD0(Pause, void());
  MOCK_METHOD1(Seek, void(base::TimeDelta time));
  MOCK_METHOD1(SetMute, void(bool mute));
  MOCK_METHOD1(SetVolume, void(float volume));

 protected:
  // The dtor is protected because MockMediaRouteController is ref-counted.
  ~MockMediaRouteController() override;
};

class MockMediaRouteControllerObserver : public MediaRouteController::Observer {
 public:
  MockMediaRouteControllerObserver(
      scoped_refptr<MediaRouteController> controller);
  ~MockMediaRouteControllerObserver() override;

  MOCK_METHOD1(OnMediaStatusUpdated, void(const MediaStatus& status));
  MOCK_METHOD0(OnControllerInvalidated, void());
};

// Tests the API call flow between the MediaRouterMojoImpl and the Media Router
// Mojo service in both directions.
class MediaRouterMojoTest : public ::testing::Test {
 public:
  MediaRouterMojoTest();
  ~MediaRouterMojoTest() override;

 protected:
  void SetUp() override;
  void TearDown() override;

  // Set the MediaRouter instance to be used by the MediaRouterFactory and
  // return it.
  virtual MediaRouterMojoImpl* SetTestingFactoryAndUse() = 0;

  // Notify media router that the provider provides a route or a sink.
  // Need to be called after the provider is registered.
  void ProvideTestRoute(mojom::MediaRouteProvider::Id provider_id,
                        const MediaRoute::Id& route_id);
  void ProvideTestSink(mojom::MediaRouteProvider::Id provider_id,
                       const MediaSink::Id& sink_id);

  // Register |mock_extension_provider_| or |mock_wired_display_provider_| with
  // |media_router_|.
  void RegisterExtensionProvider();
  void RegisterWiredDisplayProvider();

  // Tests that calling MediaRouter methods result in calls to corresponding
  // MediaRouteProvider methods.
  void TestCreateRoute();
  void TestJoinRoute(const std::string& presentation_id);
  void TestConnectRouteByRouteId();
  void TestTerminateRoute();
  void TestSendRouteMessage();
  void TestSendRouteBinaryMessage();
  void TestDetachRoute();
  void TestSearchSinks();
  void TestCreateMediaRouteController();
  void TestCreateHangoutsMediaRouteController();

  const std::string& extension_id() const { return extension_->id(); }

  MediaRouterMojoImpl* router() const { return media_router_; }

  Profile* profile() { return &profile_; }

  // Mock objects.
  MockMediaRouteProvider mock_extension_provider_;
  MockMediaRouteProvider mock_wired_display_provider_;
  MockEventPageRequestManager* request_manager_ = nullptr;

 private:
  // Helper method for RegisterExtensionProvider() and
  // RegisterWiredDisplayProvider().
  void RegisterMediaRouteProvider(mojom::MediaRouteProvider* provider,
                                  mojom::MediaRouteProvider::Id provider_id);

  content::TestBrowserThreadBundle test_thread_bundle_;
  scoped_refptr<extensions::Extension> extension_;
  TestingProfile profile_;
  MediaRouterMojoImpl* media_router_ = nullptr;
  mojo::BindingSet<mojom::MediaRouteProvider> provider_bindings_;
  std::unique_ptr<MediaRoutesObserver> routes_observer_;
  std::unique_ptr<MockMediaSinksObserver> sinks_observer_;

  DISALLOW_COPY_AND_ASSIGN(MediaRouterMojoTest);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_MOJO_MEDIA_ROUTER_MOJO_TEST_H_
