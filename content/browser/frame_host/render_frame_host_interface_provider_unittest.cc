// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <tuple>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "content/browser/interface_provider_filtering.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/navigation_simulator.h"
#include "content/test/frame_host_test_interface.mojom.h"
#include "content/test/test_render_frame_host.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace content {

namespace {

constexpr char kTestFirstURL[] = "http://example.com/foo";
constexpr char kTestFirstURLWithRef[] = "http://example.com/foo#ref";
constexpr char kTestSecondURL[] = "http://example.com/bar";

constexpr char kEventRegisterBinder[] = "RegisterBinder";
constexpr char kEventBind[] = "Bind";
constexpr char kEventDidFinishNavigation[] = "DidFinishNavigation";

// Tuple of event and last committed URL at the time of observing the event.
using ObservedEvent = std::tuple<std::string, GURL>;

class ScopedFrameHostTestInterfaceBinder : public WebContentsObserver {
 public:
  // The |frame| should outlive this.
  ScopedFrameHostTestInterfaceBinder(TestRenderFrameHost* frame)
      : WebContentsObserver(WebContents::FromRenderFrameHost(frame)),
        frame_(frame),
        was_bind_called_(false) {
    frame->binder_registry().AddInterface(base::Bind(
        &ScopedFrameHostTestInterfaceBinder::Bind, base::Unretained(this)));
    LogEvent(kEventRegisterBinder);
  }

  ~ScopedFrameHostTestInterfaceBinder() override {
    frame_->binder_registry().RemoveInterface(
        mojom::FrameHostTestInterface::Name_);
  }

  const std::vector<ObservedEvent>& observed_events() const {
    return observed_events_;
  }

  bool was_bind_called() const { return was_bind_called_; }
  void WaitForIncomingBindRequest() {
    if (was_bind_called())
      return;
    run_loop_.Run();
  }

 protected:
  // WebContentsObserver:
  void DidFinishNavigation(NavigationHandle* navigation_handle) override {
    LogEvent(kEventDidFinishNavigation);
  }

 private:
  void Bind(mojom::FrameHostTestInterfaceRequest request) {
    LogEvent(kEventBind);
    was_bind_called_ = true;
    run_loop_.Quit();
  }

  void LogEvent(base::StringPiece event) {
    observed_events_.emplace_back(event.as_string(),
                                  frame_->GetLastCommittedURL());
  }

  TestRenderFrameHost* frame_;
  std::vector<ObservedEvent> observed_events_;
  bool was_bind_called_;
  base::RunLoop run_loop_;

  DISALLOW_COPY_AND_ASSIGN(ScopedFrameHostTestInterfaceBinder);
};

}  // namespace

class RenderFrameHostInterfaceProviderTest
    : public content::RenderViewHostTestHarness {
 public:
  RenderFrameHostInterfaceProviderTest() {}
  ~RenderFrameHostInterfaceProviderTest() override {}

 protected:
  TestRenderFrameHost* GetMainFrame() {
    return static_cast<TestRenderFrameHost*>(web_contents()->GetMainFrame());
  }

  void RequestTestInterface(
      service_manager::mojom::InterfaceProvider* interface_provider) {
    mojom::FrameHostTestInterfacePtr test_interface;
    interface_provider->GetInterface(
        mojom::FrameHostTestInterface::Name_,
        mojo::MakeRequest(&test_interface).PassMessagePipe());
  }

  void SimulateNavigationAndExpectNoTransfer(
      const GURL& url,
      service_manager::mojom::InterfaceProviderRequest request) {
    TestRenderFrameHost* original_rfh = GetMainFrame();
    SimulateNavigation(url, std::move(request));
    EXPECT_EQ(GetMainFrame(), original_rfh);
  }

  void SimulateNavigation(
      const GURL& url,
      service_manager::mojom::InterfaceProviderRequest request) {
    auto navigation_simulator = NavigationSimulator::CreateRendererInitiated(
        url, web_contents()->GetMainFrame());
    navigation_simulator->SetInterfaceProviderRequest(std::move(request));
    navigation_simulator->Start();
    navigation_simulator->Commit();
  }

 private:
  test::ScopedInterfaceFilterBypass scoped_interface_filter_bypass_;

  DISALLOW_COPY_AND_ASSIGN(RenderFrameHostInterfaceProviderTest);
};

// A GetInterface message originating from the old document, but arriving
// just after DidCommitProvisionalLoad, should not be serviced.
TEST_F(RenderFrameHostInterfaceProviderTest, LateInterfaceRequest) {
  service_manager::mojom::InterfaceProviderPtr
      interface_provider_for_first_navigation;
  SimulateNavigation(
      GURL(kTestFirstURL),
      mojo::MakeRequest(&interface_provider_for_first_navigation));

  ScopedFrameHostTestInterfaceBinder scoped_binder(GetMainFrame());

  SimulateNavigationAndExpectNoTransfer(
      GURL(kTestSecondURL),
      TestRenderFrameHost::CreateIsolatedInterfaceProviderRequest());

  RequestTestInterface(interface_provider_for_first_navigation.get());

  base::RunLoop run_loop;
  interface_provider_for_first_navigation.set_connection_error_handler(
      run_loop.QuitClosure());
  run_loop.Run();

  const std::vector<ObservedEvent> expected_events = {
      {std::string(kEventRegisterBinder), GURL(kTestFirstURL)},
      {std::string(kEventDidFinishNavigation), GURL(kTestSecondURL)}};

  EXPECT_TRUE(interface_provider_for_first_navigation.encountered_error());
  EXPECT_FALSE(scoped_binder.was_bind_called());
  EXPECT_THAT(scoped_binder.observed_events(),
              testing::ElementsAreArray(expected_events));
}

// A GetInterface message originating from the new document, and being already
// pending when the new InterfaceProvider is bound in DidCommitProvisionalLoad,
// should only be dispatched after the navigation commits.
TEST_F(RenderFrameHostInterfaceProviderTest, EarlyInterfaceRequest) {
  SimulateNavigation(
      GURL(kTestFirstURL),
      TestRenderFrameHost::CreateIsolatedInterfaceProviderRequest());

  ScopedFrameHostTestInterfaceBinder scoped_binder(GetMainFrame());

  service_manager::mojom::InterfaceProviderPtr
      interface_provider_for_second_navigation;
  auto request = mojo::MakeRequest(&interface_provider_for_second_navigation);
  RequestTestInterface(interface_provider_for_second_navigation.get());

  SimulateNavigationAndExpectNoTransfer(GURL(kTestSecondURL),
                                        std::move(request));

  scoped_binder.WaitForIncomingBindRequest();

  const std::vector<ObservedEvent> expected_events = {
      {std::string(kEventRegisterBinder), GURL(kTestFirstURL)},
      {std::string(kEventDidFinishNavigation), GURL(kTestSecondURL)},
      {std::string(kEventBind), GURL(kTestSecondURL)}};

  EXPECT_FALSE(interface_provider_for_second_navigation.encountered_error());
  EXPECT_TRUE(scoped_binder.was_bind_called());
  EXPECT_THAT(scoped_binder.observed_events(),
              testing::ElementsAreArray(expected_events));
}

// Ensure that the render process is killed if it tries to commit a
// non-same-document navigation without binding a new InterfaceProvider.
TEST_F(RenderFrameHostInterfaceProviderTest, NonSameDocumentNavigation) {
  service_manager::mojom::InterfaceProviderPtr interface_provider;
  SimulateNavigation(GURL(kTestFirstURL),
                     mojo::MakeRequest(&interface_provider));

  MockRenderProcessHost* rph = GetMainFrame()->GetProcess();
  ASSERT_EQ(0, rph->bad_msg_count());
  SimulateNavigationAndExpectNoTransfer(GURL("javascript:alert()"), nullptr);
  EXPECT_EQ(1, rph->bad_msg_count());
}

// Ensure that the render process is not killed if it commits a same-document
// navigation without binding a new InterfaceProvider, and the old one continues
// to function.
TEST_F(RenderFrameHostInterfaceProviderTest, SameDocumentNavigation) {
  service_manager::mojom::InterfaceProviderPtr interface_provider;
  SimulateNavigation(GURL(kTestFirstURL),
                     mojo::MakeRequest(&interface_provider));

  ScopedFrameHostTestInterfaceBinder scoped_binder(GetMainFrame());
  auto navigation_simulator = NavigationSimulator::CreateRendererInitiated(
      GURL(kTestFirstURLWithRef), GetMainFrame());
  navigation_simulator->SetInterfaceProviderRequest(nullptr);
  navigation_simulator->CommitSameDocument();

  RequestTestInterface(interface_provider.get());

  scoped_binder.WaitForIncomingBindRequest();

  const std::vector<ObservedEvent> expected_events = {
      {std::string(kEventRegisterBinder), GURL(kTestFirstURL)},
      {std::string(kEventDidFinishNavigation), GURL(kTestFirstURLWithRef)},
      {std::string(kEventBind), GURL(kTestFirstURLWithRef)}};

  EXPECT_FALSE(interface_provider.encountered_error());
  EXPECT_TRUE(scoped_binder.was_bind_called());
  EXPECT_THAT(scoped_binder.observed_events(),
              testing::ElementsAreArray(expected_events));
}

}  // namespace content
