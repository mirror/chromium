// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/navigation_simulator.h"
#include "content/test/frame_host_test_interface.mojom.h"
#include "content/test/test_render_frame_host.h"
#include "services/service_manager/public/cpp/interface_binder.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace content {

namespace {

constexpr char kTestFirstURL[] = "http://example.com/foo";
constexpr char kTestSecondURL[] = "http://example.com/bar";

class ScopedFakeSourceAnnotatedInterfaceBinder {
 public:
  // The |frame| should outlive this.
  ScopedFakeSourceAnnotatedInterfaceBinder(TestRenderFrameHost* frame)
      : frame_(frame), was_bind_called_(false) {
    frame->binder_registry().AddInterface(
        base::Bind(&ScopedFakeSourceAnnotatedInterfaceBinder::Bind,
                   base::Unretained(this)));
  }

  ~ScopedFakeSourceAnnotatedInterfaceBinder() {
    frame_->binder_registry().RemoveInterface(
        mojom::SourceAnnotatedInterface::Name_);
  }

  bool was_bind_called() const { return was_bind_called_; }

 private:
  void Bind(mojom::SourceAnnotatedInterfaceRequest request) {
    // Just make a note, no need to actually bind anything.
    was_bind_called_ = true;
  }

  TestRenderFrameHost* frame_;
  bool was_bind_called_;

  DISALLOW_COPY_AND_ASSIGN(ScopedFakeSourceAnnotatedInterfaceBinder);
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
    mojom::SourceAnnotatedInterfacePtr test_interface;
    interface_provider->GetInterface(
        mojom::SourceAnnotatedInterface::Name_,
        mojo::MakeRequest(&test_interface).PassMessagePipe());
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
  DISALLOW_COPY_AND_ASSIGN(RenderFrameHostInterfaceProviderTest);
};

TEST_F(RenderFrameHostInterfaceProviderTest, ReplacedOnNavigation) {
  service_manager::mojom::InterfaceProviderPtr
      interface_provider_for_first_navigation;
  SimulateNavigation(
      GURL(kTestFirstURL),
      mojo::MakeRequest(&interface_provider_for_first_navigation));

  TestRenderFrameHost* main_frame = GetMainFrame();
  ScopedFakeSourceAnnotatedInterfaceBinder scoped_binder(main_frame);

  service_manager::mojom::InterfaceProviderPtr
      interface_provider_for_second_navigation;
  SimulateNavigation(
      GURL(kTestSecondURL),
      mojo::MakeRequest(&interface_provider_for_second_navigation));
  ASSERT_EQ(main_frame, GetMainFrame());

  // Simulate an interface request racing with navigation commit. The request
  // should be ignored, the pipe to the InterfaceProvider closed.
  RequestTestInterface(interface_provider_for_first_navigation.get());
  base::RunLoop run_loop;
  interface_provider_for_first_navigation.set_connection_error_handler(
      run_loop.QuitClosure());
  run_loop.Run();
  EXPECT_TRUE(interface_provider_for_first_navigation.encountered_error());
  EXPECT_FALSE(scoped_binder.was_bind_called());

  ASSERT_FALSE(interface_provider_for_second_navigation.encountered_error());
  RequestTestInterface(interface_provider_for_second_navigation.get());
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(scoped_binder.was_bind_called());
}

TEST_F(RenderFrameHostInterfaceProviderTest, NotReplacedOnNavigation) {
  service_manager::mojom::InterfaceProviderPtr interface_provider;
  SimulateNavigation(GURL(kTestFirstURL),
                     mojo::MakeRequest(&interface_provider));

  TestRenderFrameHost* main_frame = GetMainFrame();
  ScopedFakeSourceAnnotatedInterfaceBinder scoped_binder(main_frame);

  SimulateNavigation(GURL(kTestSecondURL), nullptr);
  ASSERT_EQ(main_frame, GetMainFrame());

  ASSERT_FALSE(interface_provider.encountered_error());
  RequestTestInterface(interface_provider.get());
  base::RunLoop().RunUntilIdle();
  EXPECT_TRUE(scoped_binder.was_bind_called());
}

}  // namespace content
