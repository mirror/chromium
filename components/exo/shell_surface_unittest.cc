// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/shell_surface.h"

#include "ash/accessibility/accessibility_delegate.h"
#include "ash/public/cpp/shell_window_ids.h"
#include "ash/shell.h"
#include "ash/shell_port.h"
#include "ash/shell_test_api.h"
#include "ash/wm/window_state.h"
#include "ash/wm/wm_event.h"
#include "ash/wm/workspace/workspace_window_resizer.h"
#include "ash/wm/workspace_controller_test_api.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "components/exo/buffer.h"
#include "components/exo/client_controlled_shell_surface.h"
#include "components/exo/display.h"
#include "components/exo/sub_surface.h"
#include "components/exo/surface.h"
#include "components/exo/test/exo_test_base.h"
#include "components/exo/test/exo_test_helper.h"
#include "components/exo/wm_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/aura/window.h"
#include "ui/base/hit_test.h"
#include "ui/compositor/compositor.h"
#include "ui/compositor/layer.h"
#include "ui/display/display.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/screen.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/core/shadow.h"
#include "ui/wm/core/shadow_controller.h"
#include "ui/wm/core/shadow_types.h"
#include "ui/wm/core/window_util.h"

namespace exo {
namespace {

using ShellSurfaceTest = test::ExoTestBase;

uint32_t ConfigureFullscreen(uint32_t serial,
                             const gfx::Size& size,
                             ash::mojom::WindowStateType state_type,
                             bool resizing,
                             bool activated,
                             const gfx::Vector2d& origin_offset) {
  EXPECT_EQ(ash::mojom::WindowStateType::FULLSCREEN, state_type);
  return serial;
}

wm::ShadowElevation GetShadowElevation(aura::Window* window) {
  return window->GetProperty(wm::kShadowElevationKey);
}

class ShellSurfaceBoundsModeTest
    : public ShellSurfaceTest,
      public testing::WithParamInterface<ShellSurface::BoundsMode> {
 public:
  ShellSurfaceBoundsModeTest() {}
  ~ShellSurfaceBoundsModeTest() override {}

  bool IsClientBoundsMode() const {
    return GetParam() == ShellSurface::BoundsMode::CLIENT;
  }

  bool HasBackdrop() {
    ash::WorkspaceController* wc =
        ash::ShellTestApi(ash::Shell::Get()).workspace_controller();
    return !!ash::WorkspaceControllerTestApi(wc).GetBackdropWindow();
  }

  std::unique_ptr<ShellSurface> CreateDefaultShellSurface(Surface* surface) {
    if (IsClientBoundsMode())
      return exo_test_helper()->CreateClientControlledShellSurface(surface);
    else
      return Display().CreateShellSurface(surface);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ShellSurfaceBoundsModeTest);
};

INSTANTIATE_TEST_CASE_P(,
                        ShellSurfaceBoundsModeTest,
                        testing::Values(ShellSurface::BoundsMode::CLIENT,
                                        ShellSurface::BoundsMode::SHELL));

TEST_F(ShellSurfaceTest, AcknowledgeConfigure) {
  gfx::Size buffer_size(32, 32);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));

  surface->Attach(buffer.get());
  surface->Commit();

  gfx::Point origin(100, 100);
  shell_surface->GetWidget()->SetBounds(gfx::Rect(origin, buffer_size));
  EXPECT_EQ(origin.ToString(),
            surface->window()->GetBoundsInRootWindow().origin().ToString());

  const uint32_t kSerial = 1;
  shell_surface->set_configure_callback(
      base::Bind(&ConfigureFullscreen, kSerial));
  shell_surface->SetFullscreen(true);

  // Surface origin should not change until configure request is acknowledged.
  EXPECT_EQ(origin.ToString(),
            surface->window()->GetBoundsInRootWindow().origin().ToString());

  // Compositor should be locked until configure request is acknowledged.
  ui::Compositor* compositor =
      shell_surface->GetWidget()->GetNativeWindow()->layer()->GetCompositor();
  EXPECT_TRUE(compositor->IsLocked());

  shell_surface->AcknowledgeConfigure(kSerial);
  std::unique_ptr<Buffer> fullscreen_buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(
          CurrentContext()->bounds().size())));
  surface->Attach(fullscreen_buffer.get());
  surface->Commit();

  EXPECT_EQ(gfx::Point().ToString(),
            surface->window()->GetBoundsInRootWindow().origin().ToString());
  EXPECT_FALSE(compositor->IsLocked());
}

TEST_F(ShellSurfaceTest, SetParent) {
  gfx::Size buffer_size(256, 256);
  std::unique_ptr<Buffer> parent_buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> parent_surface(new Surface);
  std::unique_ptr<ShellSurface> parent_shell_surface(
      new ShellSurface(parent_surface.get()));

  parent_surface->Attach(parent_buffer.get());
  parent_surface->Commit();

  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));
  shell_surface->SetParent(parent_shell_surface.get());

  surface->Attach(buffer.get());
  surface->Commit();
  EXPECT_EQ(
      parent_shell_surface->GetWidget()->GetNativeWindow(),
      wm::GetTransientParent(shell_surface->GetWidget()->GetNativeWindow()));
}

TEST_P(ShellSurfaceBoundsModeTest, Maximize) {
  gfx::Size buffer_size(256, 256);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(
      CreateDefaultShellSurface(surface.get()));

  surface->Attach(buffer.get());
  surface->Commit();
  EXPECT_FALSE(HasBackdrop());
  shell_surface->Maximize();
  EXPECT_EQ(IsClientBoundsMode(), HasBackdrop());
  surface->Commit();
  EXPECT_EQ(IsClientBoundsMode(), HasBackdrop());
  if (!IsClientBoundsMode()) {
    EXPECT_EQ(CurrentContext()->bounds().width(),
              shell_surface->GetWidget()->GetWindowBoundsInScreen().width());
  }
  EXPECT_TRUE(shell_surface->GetWidget()->IsMaximized());

  // Toggle maximize.
  ash::wm::WMEvent maximize_event(ash::wm::WM_EVENT_TOGGLE_MAXIMIZE);
  aura::Window* window = shell_surface->GetWidget()->GetNativeWindow();

  ash::wm::GetWindowState(window)->OnWMEvent(&maximize_event);
  EXPECT_FALSE(shell_surface->GetWidget()->IsMaximized());
  EXPECT_FALSE(HasBackdrop());

  ash::wm::GetWindowState(window)->OnWMEvent(&maximize_event);
  EXPECT_TRUE(shell_surface->GetWidget()->IsMaximized());
  EXPECT_EQ(IsClientBoundsMode(), HasBackdrop());
}

TEST_F(ShellSurfaceTest, Minimize) {
  gfx::Size buffer_size(256, 256);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));

  // Minimizing can be performed before the surface is committed.
  shell_surface->Minimize();
  EXPECT_TRUE(shell_surface->GetWidget()->IsMinimized());

  // Confirm that attaching and commiting doesn't reset the state.
  surface->Attach(buffer.get());
  surface->Commit();
  EXPECT_TRUE(shell_surface->GetWidget()->IsMinimized());

  shell_surface->Restore();
  EXPECT_FALSE(shell_surface->GetWidget()->IsMinimized());

  shell_surface->Minimize();
  EXPECT_TRUE(shell_surface->GetWidget()->IsMinimized());
}

TEST_P(ShellSurfaceBoundsModeTest, Restore) {
  gfx::Size buffer_size(256, 256);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(
      CreateDefaultShellSurface(surface.get()));

  surface->Attach(buffer.get());
  surface->Commit();
  EXPECT_FALSE(HasBackdrop());
  // Note: Remove contents to avoid issues with maximize animations in tests.
  shell_surface->Maximize();
  EXPECT_EQ(IsClientBoundsMode(), HasBackdrop());
  shell_surface->Restore();
  EXPECT_FALSE(HasBackdrop());
  if (!IsClientBoundsMode()) {
    EXPECT_EQ(buffer_size.ToString(), shell_surface->GetWidget()
                                          ->GetWindowBoundsInScreen()
                                          .size()
                                          .ToString());
  }
}

TEST_P(ShellSurfaceBoundsModeTest, SetFullscreen) {
  gfx::Size buffer_size(256, 256);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(
      CreateDefaultShellSurface(surface.get()));

  shell_surface->SetFullscreen(true);
  surface->Attach(buffer.get());
  surface->Commit();
  EXPECT_EQ(IsClientBoundsMode(), HasBackdrop());
  if (!IsClientBoundsMode()) {
    EXPECT_EQ(CurrentContext()->bounds().ToString(),
              shell_surface->GetWidget()->GetWindowBoundsInScreen().ToString());
  }

  shell_surface->SetFullscreen(false);
  surface->Commit();
  EXPECT_FALSE(HasBackdrop());
  EXPECT_NE(CurrentContext()->bounds().ToString(),
            shell_surface->GetWidget()->GetWindowBoundsInScreen().ToString());
}

TEST_F(ShellSurfaceTest, SetTitle) {
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));

  shell_surface->SetTitle(base::string16(base::ASCIIToUTF16("test")));
  surface->Commit();
}

TEST_F(ShellSurfaceTest, SetApplicationId) {
  gfx::Size buffer_size(64, 64);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));

  EXPECT_FALSE(shell_surface->GetWidget());
  shell_surface->SetApplicationId("pre-widget-id");

  surface->Attach(buffer.get());
  surface->Commit();
  aura::Window* window = shell_surface->GetWidget()->GetNativeWindow();
  EXPECT_EQ("pre-widget-id", *ShellSurface::GetApplicationId(window));
  shell_surface->SetApplicationId("test");
  EXPECT_EQ("test", *ShellSurface::GetApplicationId(window));
}

TEST_F(ShellSurfaceTest, Move) {
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));

  // Map shell surface.
  surface->Commit();

  // The interactive move should end when surface is destroyed.
  shell_surface->Move();

  // Test that destroying the shell surface before move ends is OK.
  shell_surface.reset();
}

TEST_F(ShellSurfaceTest, Resize) {
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));

  // Map shell surface.
  surface->Commit();

  // The interactive resize should end when surface is destroyed.
  shell_surface->Resize(HTBOTTOMRIGHT);

  // Test that destroying the surface before resize ends is OK.
  surface.reset();
}

TEST_F(ShellSurfaceTest, SetGeometry) {
  gfx::Size buffer_size(64, 64);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));

  gfx::Rect geometry(16, 16, 32, 32);
  shell_surface->SetGeometry(geometry);
  surface->Attach(buffer.get());
  surface->Commit();
  EXPECT_EQ(
      geometry.size().ToString(),
      shell_surface->GetWidget()->GetWindowBoundsInScreen().size().ToString());
  EXPECT_EQ(gfx::Rect(gfx::Point() - geometry.OffsetFromOrigin(), buffer_size)
                .ToString(),
            shell_surface->host_window()->bounds().ToString());
}

TEST_F(ShellSurfaceTest, SetMinimumSize) {
  gfx::Size buffer_size(64, 64);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));

  gfx::Size size(50, 50);
  shell_surface->SetMinimumSize(size);
  surface->Attach(buffer.get());
  surface->Commit();
  EXPECT_EQ(size, shell_surface->GetMinimumSize());
}

TEST_F(ShellSurfaceTest, SetMaximumSize) {
  gfx::Size buffer_size(64, 64);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));

  gfx::Size size(100, 100);
  shell_surface->SetMaximumSize(size);
  surface->Attach(buffer.get());
  surface->Commit();
  EXPECT_EQ(size, shell_surface->GetMaximumSize());
}

TEST_P(ShellSurfaceBoundsModeTest, DefaultDeviceScaleFactorForcedScaleFactor) {
  double scale = 1.5;
  display::Display::SetForceDeviceScaleFactor(scale);

  int64_t display_id = display::Screen::GetScreen()->GetPrimaryDisplay().id();
  display::Display::SetInternalDisplayId(display_id);

  gfx::Size buffer_size(64, 64);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(
      CreateDefaultShellSurface(surface.get()));
  surface->Attach(buffer.get());
  surface->Commit();
  gfx::Transform transform;
  if (IsClientBoundsMode())
    transform.Scale(1.0 / scale, 1.0 / scale);

  EXPECT_EQ(
      transform.ToString(),
      shell_surface->host_window()->layer()->GetTargetTransform().ToString());
}

TEST_P(ShellSurfaceBoundsModeTest, DefaultDeviceScaleFactorFromDisplayManager) {
  int64_t display_id = display::Screen::GetScreen()->GetPrimaryDisplay().id();
  display::Display::SetInternalDisplayId(display_id);
  gfx::Size size(1920, 1080);

  display::DisplayManager* display_manager =
      ash::Shell::Get()->display_manager();

  double scale = 1.25;
  display::ManagedDisplayMode mode(size, 60.f, false /* overscan */,
                                   true /*native*/, 1.0, scale);
  mode.set_is_default(true);

  display::ManagedDisplayInfo::ManagedDisplayModeList mode_list;
  mode_list.push_back(mode);

  display::ManagedDisplayInfo native_display_info(display_id, "test", false);
  native_display_info.SetManagedDisplayModes(mode_list);

  native_display_info.SetBounds(gfx::Rect(size));
  native_display_info.set_device_scale_factor(scale);

  std::vector<display::ManagedDisplayInfo> display_info_list;
  display_info_list.push_back(native_display_info);

  display_manager->OnNativeDisplaysChanged(display_info_list);
  display_manager->UpdateInternalManagedDisplayModeListForTest();

  gfx::Size buffer_size(64, 64);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(
      CreateDefaultShellSurface(surface.get()));

  surface->Attach(buffer.get());
  surface->Commit();

  gfx::Transform transform;
  if (IsClientBoundsMode())
    transform.Scale(1.0 / scale, 1.0 / scale);

  EXPECT_EQ(
      transform.ToString(),
      shell_surface->host_window()->layer()->GetTargetTransform().ToString());
}

void Close(int* close_call_count) {
  (*close_call_count)++;
}

TEST_F(ShellSurfaceTest, CloseCallback) {
  gfx::Size buffer_size(64, 64);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));

  int close_call_count = 0;
  shell_surface->set_close_callback(
      base::Bind(&Close, base::Unretained(&close_call_count)));

  surface->Attach(buffer.get());
  surface->Commit();

  EXPECT_EQ(0, close_call_count);
  shell_surface->GetWidget()->Close();
  EXPECT_EQ(1, close_call_count);
}

void DestroyShellSurface(std::unique_ptr<ShellSurface>* shell_surface) {
  shell_surface->reset();
}

TEST_F(ShellSurfaceTest, SurfaceDestroyedCallback) {
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));

  shell_surface->set_surface_destroyed_callback(
      base::Bind(&DestroyShellSurface, base::Unretained(&shell_surface)));

  surface->Commit();

  EXPECT_TRUE(shell_surface.get());
  surface.reset();
  EXPECT_FALSE(shell_surface.get());
}

uint32_t Configure(gfx::Size* suggested_size,
                   ash::mojom::WindowStateType* has_state_type,
                   bool* is_resizing,
                   bool* is_active,
                   const gfx::Size& size,
                   ash::mojom::WindowStateType state_type,
                   bool resizing,
                   bool activated,
                   const gfx::Vector2d& origin_offset) {
  *suggested_size = size;
  *has_state_type = state_type;
  *is_resizing = resizing;
  *is_active = activated;
  return 0;
}

TEST_F(ShellSurfaceTest, ConfigureCallback) {
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(surface.get()));

  gfx::Size suggested_size;
  ash::mojom::WindowStateType has_state_type =
      ash::mojom::WindowStateType::NORMAL;
  bool is_resizing = false;
  bool is_active = false;
  shell_surface->set_configure_callback(
      base::Bind(&Configure, base::Unretained(&suggested_size),
                 base::Unretained(&has_state_type),
                 base::Unretained(&is_resizing), base::Unretained(&is_active)));
  // Commit without contents should result in a configure callback with empty
  // suggested size as a mechanims to ask the client size itself.
  surface->Commit();
  EXPECT_EQ(gfx::Size(), suggested_size);

  shell_surface->Maximize();
  shell_surface->AcknowledgeConfigure(0);
  EXPECT_EQ(CurrentContext()->bounds().width(), suggested_size.width());
  EXPECT_EQ(ash::mojom::WindowStateType::MAXIMIZED, has_state_type);
  shell_surface->Restore();
  shell_surface->AcknowledgeConfigure(0);

  shell_surface->SetFullscreen(true);
  shell_surface->AcknowledgeConfigure(0);
  EXPECT_EQ(CurrentContext()->bounds().size().ToString(),
            suggested_size.ToString());
  EXPECT_EQ(ash::mojom::WindowStateType::FULLSCREEN, has_state_type);
  shell_surface->SetFullscreen(false);
  shell_surface->AcknowledgeConfigure(0);

  gfx::Size buffer_size(64, 64);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  surface->Attach(buffer.get());
  surface->Commit();
  shell_surface->GetWidget()->Activate();
  shell_surface->AcknowledgeConfigure(0);
  EXPECT_TRUE(is_active);
  shell_surface->GetWidget()->Deactivate();
  shell_surface->AcknowledgeConfigure(0);
  EXPECT_FALSE(is_active);

  EXPECT_FALSE(is_resizing);
  shell_surface->Resize(HTBOTTOMRIGHT);
  shell_surface->AcknowledgeConfigure(0);
  EXPECT_TRUE(is_resizing);
}

TEST_F(ShellSurfaceTest, NonSurfaceShadow) {
  gfx::Size buffer_size(128, 128);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(new ShellSurface(
      surface.get(), ShellSurface::BoundsMode::SHELL, gfx::Point(), true, false,
      ash::kShellWindowId_DefaultContainer));
  surface->Attach(buffer.get());
  surface->Commit();

  aura::Window* window = shell_surface->GetWidget()->GetNativeWindow();

  // 1) Initial state, no shadow.
  wm::Shadow* shadow = wm::ShadowController::GetShadowForWindow(window);
  ASSERT_TRUE(shadow);
  EXPECT_FALSE(shadow->layer()->visible());

  std::unique_ptr<Display> display(new Display);

  // 2) Just creating a sub surface won't create a shadow.
  std::unique_ptr<Surface> child = display->CreateSurface();
  std::unique_ptr<Buffer> child_buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  child->Attach(child_buffer.get());
  std::unique_ptr<SubSurface> sub_surface(
      display->CreateSubSurface(child.get(), surface.get()));
  surface->Commit();

  EXPECT_FALSE(shadow->layer()->visible());

  // 3) Enable a shadow.
  shell_surface->OnSetFrame(SurfaceFrameType::SHADOW);
  surface->Commit();
  EXPECT_TRUE(shadow->layer()->visible());

  gfx::Rect before = shadow->layer()->bounds();

  // 4) Shadow bounds is independent of the sub surface.
  gfx::Size new_buffer_size(256, 256);
  std::unique_ptr<Buffer> new_child_buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(new_buffer_size)));
  child->Attach(new_child_buffer.get());
  child->Commit();
  surface->Commit();

  EXPECT_EQ(before, shadow->layer()->bounds());

  // 4) Updating the widget's window bounds should change the non surface shadow
  // bounds.
  const gfx::Rect new_bounds(50, 50, 100, 100);
  window->SetBounds(new_bounds);
  EXPECT_NE(before, shadow->layer()->bounds());
  EXPECT_NE(new_bounds, shadow->layer()->bounds());

  // 5) This should disable shadow.
  surface->SetFrame(SurfaceFrameType::NONE);
  surface->Commit();

  EXPECT_EQ(wm::ShadowElevation::NONE, GetShadowElevation(window));
  EXPECT_FALSE(shadow->layer()->visible());

  // 6) This should enable non surface shadow.
  surface->SetFrame(SurfaceFrameType::SHADOW);
  surface->Commit();

  EXPECT_EQ(wm::ShadowElevation::DEFAULT, GetShadowElevation(window));
  EXPECT_TRUE(shadow->layer()->visible());
}

TEST_P(ShellSurfaceBoundsModeTest, ToggleFullscreen) {
  gfx::Size buffer_size(256, 256);
  std::unique_ptr<Buffer> buffer(
      new Buffer(exo_test_helper()->CreateGpuMemoryBuffer(buffer_size)));
  std::unique_ptr<Surface> surface(new Surface);
  std::unique_ptr<ShellSurface> shell_surface(
      CreateDefaultShellSurface(surface.get()));

  surface->Attach(buffer.get());
  surface->Commit();
  EXPECT_FALSE(HasBackdrop());
  if (!IsClientBoundsMode()) {
    EXPECT_EQ(buffer_size.ToString(), shell_surface->GetWidget()
                                          ->GetWindowBoundsInScreen()
                                          .size()
                                          .ToString());
  }
  shell_surface->Maximize();
  EXPECT_EQ(IsClientBoundsMode(), HasBackdrop());
  if (!IsClientBoundsMode()) {
    EXPECT_EQ(CurrentContext()->bounds().width(),
              shell_surface->GetWidget()->GetWindowBoundsInScreen().width());
  }

  ash::wm::WMEvent event(ash::wm::WM_EVENT_TOGGLE_FULLSCREEN);
  aura::Window* window = shell_surface->GetWidget()->GetNativeWindow();

  // Enter fullscreen mode.
  ash::wm::GetWindowState(window)->OnWMEvent(&event);

  EXPECT_EQ(IsClientBoundsMode(), HasBackdrop());
  if (!IsClientBoundsMode()) {
    EXPECT_EQ(CurrentContext()->bounds().ToString(),
              shell_surface->GetWidget()->GetWindowBoundsInScreen().ToString());
  }

  // Leave fullscreen mode.
  ash::wm::GetWindowState(window)->OnWMEvent(&event);
  EXPECT_EQ(IsClientBoundsMode(), HasBackdrop());

  // Check that shell surface is maximized.
  if (!IsClientBoundsMode()) {
    EXPECT_EQ(CurrentContext()->bounds().width(),
              shell_surface->GetWidget()->GetWindowBoundsInScreen().width());
  }
}

}  // namespace
}  // namespace exo
