// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/resource_coordinator/window_occlusion_tracker.h"

#include <memory>

#include "base/run_loop.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/resource_coordinator/window_occlusion_observer.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/exo/buffer.h"
#include "components/exo/shell_surface.h"
#include "components/exo/test/exo_test_helper.h"
#include "components/exo/wm_helper_ash.h"
#include "gpu/command_buffer/client/gpu_memory_buffer_manager.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/env.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/widget/widget.h"

namespace resource_coordinator {

namespace {

class MockWindowOcclusionObserver : public WindowOcclusionObserver {
 public:
  MockWindowOcclusionObserver() = default;
  MOCK_METHOD2(OnWindowOcclusionStateChanged, void(gfx::NativeWindow, bool));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockWindowOcclusionObserver);
};

class WindowOcclusionTrackerTest : public InProcessBrowserTest {
 protected:
  WindowOcclusionTrackerTest() = default;

  void PreRunTestOnMainThread() override {
    InProcessBrowserTest::PreRunTestOnMainThread();

    window_occlusion_tracker_ = std::make_unique<WindowOcclusionTracker>();

    // Create 4 non-overlapping browser windows.
    browser1_ = CreateBrowser(ProfileManager::GetActiveUserProfile());
    browser2_ = CreateBrowser(ProfileManager::GetActiveUserProfile());
    browser3_ = CreateBrowser(ProfileManager::GetActiveUserProfile());
    browser4_ = CreateBrowser(ProfileManager::GetActiveUserProfile());

    window1_ = browser1_->window()->GetNativeWindow();
    window2_ = browser2_->window()->GetNativeWindow();
    window3_ = browser3_->window()->GetNativeWindow();
    window4_ = browser4_->window()->GetNativeWindow();

    browser1_->window()->SetBounds(gfx::Rect(0, 0, 350, 350));
    browser2_->window()->SetBounds(gfx::Rect(350, 0, 350, 350));
    browser3_->window()->SetBounds(gfx::Rect(0, 350, 350, 350));
    browser4_->window()->SetBounds(gfx::Rect(350, 350, 0, 0));

    // Wait for windows to be created.
    base::RunLoop().RunUntilIdle();

    // Add observers that will be notified when occlusion state changes.
    window_occlusion_tracker_->AddObserver(window1_, &observer1_);
    window_occlusion_tracker_->AddObserver(window2_, &observer2_);
    window_occlusion_tracker_->AddObserver(window3_, &observer3_);
    window_occlusion_tracker_->AddObserver(window4_, &observer4_);
  }

  void PostRunTestOnMainThread() {
    // Remove observers, to avoid getting window occlusion state change
    // notifications during shutdown.
    window_occlusion_tracker_->RemoveObserver(window1_, &observer1_);
    window_occlusion_tracker_->RemoveObserver(window2_, &observer2_);
    window_occlusion_tracker_->RemoveObserver(window3_, &observer3_);
    window_occlusion_tracker_->RemoveObserver(window4_, &observer4_);

    InProcessBrowserTest::PostRunTestOnMainThread();
  }

  void VerifyAndClearObservers() {
    EXPECT_TRUE(testing::Mock::VerifyAndClear(&observer1_));
    EXPECT_TRUE(testing::Mock::VerifyAndClear(&observer2_));
    EXPECT_TRUE(testing::Mock::VerifyAndClear(&observer3_));
    EXPECT_TRUE(testing::Mock::VerifyAndClear(&observer4_));
  }

  std::unique_ptr<WindowOcclusionTracker> window_occlusion_tracker_;

  Browser* browser1_ = nullptr;
  Browser* browser2_ = nullptr;
  Browser* browser3_ = nullptr;
  Browser* browser4_ = nullptr;

  gfx::NativeWindow window1_ = nullptr;
  gfx::NativeWindow window2_ = nullptr;
  gfx::NativeWindow window3_ = nullptr;
  gfx::NativeWindow window4_ = nullptr;

  testing::StrictMock<MockWindowOcclusionObserver> observer1_;
  testing::StrictMock<MockWindowOcclusionObserver> observer2_;
  testing::StrictMock<MockWindowOcclusionObserver> observer3_;
  testing::StrictMock<MockWindowOcclusionObserver> observer4_;

 private:
  DISALLOW_COPY_AND_ASSIGN(WindowOcclusionTrackerTest);
};

}  // namespace

IN_PROC_BROWSER_TEST_F(WindowOcclusionTrackerTest, MaximizeMinimizeMove) {
  // Maximize |window4|. All other windows should be occluded.
  EXPECT_CALL(observer1_, OnWindowOcclusionStateChanged(window1_, true));
  EXPECT_CALL(observer2_, OnWindowOcclusionStateChanged(window2_, true));
  EXPECT_CALL(observer3_, OnWindowOcclusionStateChanged(window3_, true));
  browser4_->window()->Maximize();
  base::RunLoop().RunUntilIdle();
  VerifyAndClearObservers();

  // Minimize |window4|. It should become occluded and all other windows should
  // become unoccluded.
  EXPECT_CALL(observer1_, OnWindowOcclusionStateChanged(window1_, false));
  EXPECT_CALL(observer2_, OnWindowOcclusionStateChanged(window2_, false));
  EXPECT_CALL(observer3_, OnWindowOcclusionStateChanged(window3_, false));
  EXPECT_CALL(observer4_, OnWindowOcclusionStateChanged(window4_, true));
  browser4_->window()->Minimize();
  base::RunLoop().RunUntilIdle();
  VerifyAndClearObservers();

  // Move |window2| on top of |window1|. |window1| should become occluded.
  EXPECT_CALL(observer1_, OnWindowOcclusionStateChanged(window1_, true));
  browser2_->window()->SetBounds(browser1_->window()->GetBounds());
  base::RunLoop().RunUntilIdle();
  VerifyAndClearObservers();
}

IN_PROC_BROWSER_TEST_F(WindowOcclusionTrackerTest, ExoShellSurface) {
  exo::WMHelperAsh wm_helper_ash;
  exo::WMHelper::SetInstance(&wm_helper_ash);

  {
    // Create an ExoShellSurface with an empty input region on top of
    // |window1_|. The occlusion state of |window1_| shouldn't change.
    exo::Buffer buffer(aura::Env::GetInstance()
                           ->context_factory()
                           ->GetGpuMemoryBufferManager()
                           ->CreateGpuMemoryBuffer(gfx::Size(1, 1),
                                                   gfx::BufferFormat::RGBA_8888,
                                                   gfx::BufferUsage::GPU_READ,
                                                   gpu::kNullSurfaceHandle));
    exo::Surface surface;
    exo::ShellSurface shell_surface(&surface);
    surface.Attach(&buffer);
    surface.SetInputRegion(SkRegion());
    surface.Commit();
    shell_surface.GetWidget()->GetNativeWindow()->SetBounds(window1_->bounds());
    base::RunLoop().RunUntilIdle();
  }

  exo::WMHelper::SetInstance(nullptr);
}

IN_PROC_BROWSER_TEST_F(WindowOcclusionTrackerTest, CoverMirroredWindow) {
  // Set the aura::client::kMirroringEnabledKey property on |window1_|. Verify
  // that it isn't occluded when |window2_| covers it.
  window1_->SetProperty(aura::client::kMirroringEnabledKey, true);
  window2_->SetBounds(window1_->bounds());
  base::RunLoop().RunUntilIdle();
}

IN_PROC_BROWSER_TEST_F(WindowOcclusionTrackerTest, MirrorCoveredWindow) {
  // Cover |window1_| with |window2_|. It should become occluded.
  EXPECT_CALL(observer1_, OnWindowOcclusionStateChanged(window1_, true));
  window2_->SetBounds(window1_->bounds());
  base::RunLoop().RunUntilIdle();
  VerifyAndClearObservers();

  // Set the aura::client::kMirroringEnabledKey property on |window1_|. It
  // should become unoccluded.
  EXPECT_CALL(observer1_, OnWindowOcclusionStateChanged(window1_, false));
  window1_->SetProperty(aura::client::kMirroringEnabledKey, true);
  base::RunLoop().RunUntilIdle();
  VerifyAndClearObservers();
}

}  // namespace resource_coordinator
