// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/mus/test/wm_test_base.h"

#include <algorithm>
#include <vector>

#include "ash/mus/bridge/wm_window_mus_test_api.h"
#include "ash/mus/root_window_controller.h"
#include "ash/mus/test/wm_test_helper.h"
#include "ash/mus/window_manager.h"
#include "ash/mus/window_manager_application.h"
#include "services/ui/public/cpp/property_type_converters.h"
#include "ui/aura/mus/property_converter.h"
#include "ui/aura/mus/window_tree_client.h"
#include "ui/aura/window.h"
#include "ui/compositor/scoped_animation_duration_scale_mode.h"
#include "ui/display/display.h"

namespace ash {
namespace mus {
namespace {

ui::mojom::WindowType MusWindowTypeFromWmWindowType(
    ui::wm::WindowType wm_window_type) {
  switch (wm_window_type) {
    case ui::wm::WINDOW_TYPE_UNKNOWN:
      break;

    case ui::wm::WINDOW_TYPE_NORMAL:
      return ui::mojom::WindowType::WINDOW;

    case ui::wm::WINDOW_TYPE_POPUP:
      return ui::mojom::WindowType::POPUP;

    case ui::wm::WINDOW_TYPE_CONTROL:
      return ui::mojom::WindowType::CONTROL;

    case ui::wm::WINDOW_TYPE_PANEL:
      return ui::mojom::WindowType::PANEL;

    case ui::wm::WINDOW_TYPE_MENU:
      return ui::mojom::WindowType::MENU;

    case ui::wm::WINDOW_TYPE_TOOLTIP:
      return ui::mojom::WindowType::TOOLTIP;
  }

  NOTREACHED();
  return ui::mojom::WindowType::CONTROL;
}

}  // namespace

WmTestBase::WmTestBase() {}

WmTestBase::~WmTestBase() {
  CHECK(setup_called_)
      << "You have overridden SetUp but never called WmTestBase::SetUp";
  CHECK(teardown_called_)
      << "You have overridden TearDown but never called WmTestBase::TearDown";
}

bool WmTestBase::SupportsMultipleDisplays() const {
  return true;
}

void WmTestBase::UpdateDisplay(const std::string& display_spec) {
  test_helper_->UpdateDisplay(display_spec);
}

aura::Window* WmTestBase::GetPrimaryRootWindow() {
  std::vector<RootWindowController*> roots =
      test_helper_->GetRootsOrderedByDisplayId();
  DCHECK(!roots.empty());
  return roots[0]->root();
}

aura::Window* WmTestBase::GetSecondaryRootWindow() {
  std::vector<RootWindowController*> roots =
      test_helper_->GetRootsOrderedByDisplayId();
  return roots.size() < 2 ? nullptr : roots[1]->root();
}

display::Display WmTestBase::GetPrimaryDisplay() {
  std::vector<RootWindowController*> roots =
      test_helper_->GetRootsOrderedByDisplayId();
  DCHECK(!roots.empty());
  return roots[0]->display();
}

display::Display WmTestBase::GetSecondaryDisplay() {
  std::vector<RootWindowController*> roots =
      test_helper_->GetRootsOrderedByDisplayId();
  return roots.size() < 2 ? display::Display() : roots[1]->display();
}

aura::Window* WmTestBase::CreateTestWindow(const gfx::Rect& bounds) {
  return CreateTestWindow(bounds, ui::wm::WINDOW_TYPE_NORMAL);
}

aura::Window* WmTestBase::CreateTestWindow(const gfx::Rect& bounds,
                                           ui::wm::WindowType window_type) {
  std::map<std::string, std::vector<uint8_t>> properties;
  if (!bounds.IsEmpty()) {
    properties[ui::mojom::WindowManager::kBounds_InitProperty] =
        mojo::ConvertTo<std::vector<uint8_t>>(bounds);
  }

  properties[ui::mojom::WindowManager::kResizeBehavior_Property] =
      mojo::ConvertTo<std::vector<uint8_t>>(
          static_cast<aura::PropertyConverter::PrimitiveType>(
              ui::mojom::kResizeBehaviorCanResize |
              ui::mojom::kResizeBehaviorCanMaximize |
              ui::mojom::kResizeBehaviorCanMinimize));

  const ui::mojom::WindowType mus_window_type =
      MusWindowTypeFromWmWindowType(window_type);
  aura::Window* window = test_helper_->GetRootsOrderedByDisplayId()[0]
                             ->window_manager()
                             ->NewTopLevelWindow(mus_window_type, &properties);
  window->Show();
  return window;
}

aura::Window* WmTestBase::CreateFullscreenTestWindow(int64_t display_id) {
  std::map<std::string, std::vector<uint8_t>> properties;
  properties[ui::mojom::WindowManager::kShowState_Property] =
      mojo::ConvertTo<std::vector<uint8_t>>(
          static_cast<aura::PropertyConverter::PrimitiveType>(
              ui::mojom::ShowState::FULLSCREEN));
  if (display_id != display::kInvalidDisplayId) {
    properties[ui::mojom::WindowManager::kDisplayId_InitProperty] =
        mojo::ConvertTo<std::vector<uint8_t>>(display_id);
  }
  aura::Window* window =
      test_helper_->GetRootsOrderedByDisplayId()[0]
          ->window_manager()
          ->NewTopLevelWindow(ui::mojom::WindowType::WINDOW, &properties);
  window->Show();
  return window;
}

aura::Window* WmTestBase::CreateChildTestWindow(aura::Window* parent,
                                                const gfx::Rect& bounds) {
  std::map<std::string, std::vector<uint8_t>> properties;
  aura::Window* window = new aura::Window(nullptr);
  window->Init(ui::LAYER_TEXTURED);
  window->SetBounds(bounds);
  window->Show();
  parent->AddChild(window);
  return window;
}

void WmTestBase::SetUp() {
  setup_called_ = true;
  // Disable animations during tests.
  zero_duration_mode_ = base::MakeUnique<ui::ScopedAnimationDurationScaleMode>(
      ui::ScopedAnimationDurationScaleMode::ZERO_DURATION);
  // Most tests expect a minimum size of 0x0.
  minimum_size_lock_ =
      base::MakeUnique<WmWindowMusTestApi::GlobalMinimumSizeLock>();
  test_helper_.reset(new WmTestHelper);
  test_helper_->Init();
}

void WmTestBase::TearDown() {
  teardown_called_ = true;
  test_helper_.reset();
  minimum_size_lock_.reset();
  zero_duration_mode_.reset();
}

}  // namespace mus
}  // namespace ash
