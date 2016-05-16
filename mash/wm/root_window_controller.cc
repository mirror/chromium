// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mash/wm/root_window_controller.h"

#include <stdint.h>

#include <map>
#include <sstream>

#include "base/bind.h"
#include "components/mus/common/event_matcher_util.h"
#include "components/mus/common/util.h"
#include "components/mus/public/cpp/property_type_converters.h"
#include "components/mus/public/cpp/window.h"
#include "components/mus/public/cpp/window_tree_connection.h"
#include "components/mus/public/cpp/window_tree_host_factory.h"
#include "components/mus/public/interfaces/window_manager.mojom.h"
#include "mash/session/public/interfaces/session.mojom.h"
#include "mash/wm/background_layout.h"
#include "mash/wm/container_ids.h"
#include "mash/wm/fill_layout.h"
#include "mash/wm/screenlock_layout.h"
#include "mash/wm/shadow_controller.h"
#include "mash/wm/shelf_layout_manager.h"
#include "mash/wm/status_layout_manager.h"
#include "mash/wm/window_layout.h"
#include "mash/wm/window_manager.h"
#include "mash/wm/window_manager_application.h"
#include "mojo/public/cpp/bindings/type_converter.h"
#include "services/shell/public/cpp/connector.h"
#include "ui/mojo/display/display_type_converters.h"

namespace mash {
namespace wm {
namespace {

const uint32_t kWindowSwitchAccelerator = 1;

void AssertTrue(bool success) {
  DCHECK(success);
}

int ContainerToLocalId(mojom::Container container) {
  return static_cast<int>(container);
}

}  // namespace

// static
RootWindowController* RootWindowController::CreateFromDisplay(
    WindowManagerApplication* app,
    mus::mojom::DisplayPtr display,
    mojo::InterfaceRequest<mus::mojom::WindowTreeClient> client_request) {
  RootWindowController* controller = new RootWindowController(app);
  controller->display_ = display.To<display::Display>();
  mus::WindowTreeConnection::CreateForWindowManager(
      controller, std::move(client_request),
      mus::WindowTreeConnection::CreateType::DONT_WAIT_FOR_EMBED,
      controller->window_manager_.get());
  return controller;
}

void RootWindowController::Destroy() {
  // See class description for details on lifetime.
  if (root_) {
    delete root_->connection();
  } else {
    // This case only happens if we're destroyed before OnEmbed().
    delete this;
  }
}

shell::Connector* RootWindowController::GetConnector() {
  return app_->connector();
}

mus::Window* RootWindowController::GetWindowForContainer(
    mojom::Container container) {
  return root_->GetChildByLocalId(ContainerToLocalId(container));
}

bool RootWindowController::WindowIsContainer(const mus::Window* window) const {
  return window &&
         window->local_id() > ContainerToLocalId(mojom::Container::ROOT) &&
         window->local_id() < ContainerToLocalId(mojom::Container::COUNT);
}

mus::WindowManagerClient* RootWindowController::window_manager_client() {
  return window_manager_->window_manager_client();
}

void RootWindowController::OnAccelerator(uint32_t id, const ui::Event& event) {
  switch (id) {
    case kWindowSwitchAccelerator:
      window_manager_client()->ActivateNextWindow();
      break;
    default:
      app_->OnAccelerator(id, event);
      break;
  }
}

ShelfLayoutManager* RootWindowController::GetShelfLayoutManager() {
  return static_cast<ShelfLayoutManager*>(
      layout_managers_[GetWindowForContainer(
                           mojom::Container::USER_PRIVATE_SHELF)]
          .get());
}

StatusLayoutManager* RootWindowController::GetStatusLayoutManager() {
  return static_cast<StatusLayoutManager*>(
      layout_managers_[GetWindowForContainer(mojom::Container::STATUS)].get());
}

RootWindowController::RootWindowController(WindowManagerApplication* app)
    : app_(app), root_(nullptr), window_count_(0) {
  window_manager_.reset(new WindowManager);
}

RootWindowController::~RootWindowController() {}

void RootWindowController::AddAccelerators() {
  window_manager_client()->AddAccelerator(
      kWindowSwitchAccelerator,
      mus::CreateKeyMatcher(mus::mojom::KeyboardCode::TAB,
                            mus::mojom::kEventFlagControlDown),
      base::Bind(&AssertTrue));
}

void RootWindowController::OnEmbed(mus::Window* root) {
  root_ = root;
  root_->set_local_id(ContainerToLocalId(mojom::Container::ROOT));
  root_->AddObserver(this);
  layout_managers_[root_].reset(new FillLayout(root_));

  app_->OnRootWindowControllerGotRoot(this);

  CreateContainers();

  for (size_t i = 0; i < kNumActivationContainers; ++i) {
    window_manager_client()->AddActivationParent(
        GetWindowForContainer(kActivationContainers[i]));
  }

  AddAccelerators();

  window_manager_->Initialize(this, app_->session());

  shadow_controller_.reset(new ShadowController(root->connection()));

  app_->OnRootWindowControllerDoneInit(this);
}

void RootWindowController::OnConnectionLost(
    mus::WindowTreeConnection* connection) {
  shadow_controller_.reset();
  delete this;
}

void RootWindowController::OnEventObserved(const ui::Event& event,
                                           mus::Window* target) {
  // Does not use EventObservers.
}

void RootWindowController::OnWindowDestroyed(mus::Window* window) {
  DCHECK_EQ(window, root_);
  app_->OnRootWindowDestroyed(this);
  root_->RemoveObserver(this);
  // Delete the |window_manager_| here so that WindowManager doesn't have to
  // worry about the possibility of |root_| being null.
  window_manager_.reset();
  root_ = nullptr;
}

void RootWindowController::CreateContainer(
    mash::wm::mojom::Container container,
    mash::wm::mojom::Container parent_container) {
  // Set the window's name to the container name (e.g. "Container::LOGIN"),
  // which makes the window hierarchy easier to read.
  std::map<std::string, std::vector<uint8_t>> properties;
  std::ostringstream container_name;
  container_name << container;
  properties[mus::mojom::WindowManager::kName_Property] =
      mojo::ConvertTo<std::vector<uint8_t>>(container_name.str());

  mus::Window* window = root_->connection()->NewWindow(&properties);
  window->set_local_id(ContainerToLocalId(container));
  layout_managers_[window].reset(new FillLayout(window));

  // User private windows are hidden by default until the window manager learns
  // the lock state, so their contents are never accidentally revealed.
  window->SetVisible(container != mojom::Container::USER_PRIVATE);
  mus::Window* parent =
      root_->GetChildByLocalId(ContainerToLocalId(parent_container));
  parent->AddChild(window);
}

void RootWindowController::CreateContainers() {
  CreateContainer(mojom::Container::ALL_USER_BACKGROUND,
                  mojom::Container::ROOT);
  CreateContainer(mojom::Container::USER, mojom::Container::ROOT);
  CreateContainer(mojom::Container::USER_BACKGROUND, mojom::Container::USER);
  CreateContainer(mojom::Container::USER_PRIVATE, mojom::Container::USER);
  CreateContainer(mojom::Container::USER_PRIVATE_WINDOWS,
                  mojom::Container::USER_PRIVATE);
  CreateContainer(mojom::Container::USER_PRIVATE_ALWAYS_ON_TOP_WINDOWS,
                  mojom::Container::USER_PRIVATE);
  CreateContainer(mojom::Container::USER_PRIVATE_DOCKED_WINDOWS,
                  mojom::Container::USER_PRIVATE);
  CreateContainer(mojom::Container::USER_PRIVATE_PRESENTATION_WINDOWS,
                  mojom::Container::USER_PRIVATE);
  CreateContainer(mojom::Container::USER_PRIVATE_SHELF,
                  mojom::Container::USER_PRIVATE);
  CreateContainer(mojom::Container::USER_PRIVATE_PANELS,
                  mojom::Container::USER_PRIVATE);
  CreateContainer(mojom::Container::USER_PRIVATE_APP_LIST,
                  mojom::Container::USER_PRIVATE);
  CreateContainer(mojom::Container::USER_PRIVATE_SYSTEM_MODAL,
                  mojom::Container::USER_PRIVATE);
  CreateContainer(mojom::Container::LOGIN, mojom::Container::ROOT);
  CreateContainer(mojom::Container::LOGIN_WINDOWS, mojom::Container::LOGIN);
  CreateContainer(mojom::Container::LOGIN_APP, mojom::Container::LOGIN);
  CreateContainer(mojom::Container::LOGIN_SHELF, mojom::Container::LOGIN);
  CreateContainer(mojom::Container::STATUS, mojom::Container::ROOT);
  CreateContainer(mojom::Container::BUBBLES, mojom::Container::ROOT);
  CreateContainer(mojom::Container::SYSTEM_MODAL_WINDOWS,
                  mojom::Container::ROOT);
  CreateContainer(mojom::Container::KEYBOARD, mojom::Container::ROOT);
  CreateContainer(mojom::Container::MENUS, mojom::Container::ROOT);
  CreateContainer(mojom::Container::DRAG_AND_TOOLTIPS, mojom::Container::ROOT);

  // Override the default layout managers for certain containers.
  mus::Window* user_background =
      GetWindowForContainer(mojom::Container::USER_BACKGROUND);
  layout_managers_[user_background].reset(
      new BackgroundLayout(user_background));

  mus::Window* login_app = GetWindowForContainer(mojom::Container::LOGIN_APP);
  layout_managers_[login_app].reset(new ScreenlockLayout(login_app));

  mus::Window* user_shelf =
      GetWindowForContainer(mojom::Container::USER_PRIVATE_SHELF);
  layout_managers_[user_shelf].reset(new ShelfLayoutManager(user_shelf));

  mus::Window* status = GetWindowForContainer(mojom::Container::STATUS);
  layout_managers_[status].reset(new StatusLayoutManager(status));

  mus::Window* user_private_windows =
      GetWindowForContainer(mojom::Container::USER_PRIVATE_WINDOWS);
  layout_managers_[user_private_windows].reset(
      new WindowLayout(user_private_windows));
}

}  // namespace wm
}  // namespace mash
