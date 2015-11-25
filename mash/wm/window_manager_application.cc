// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mash/wm/window_manager_application.h"

#include "components/mus/common/util.h"
#include "components/mus/public/cpp/event_matcher.h"
#include "components/mus/public/cpp/window.h"
#include "components/mus/public/cpp/window_tree_connection.h"
#include "components/mus/public/cpp/window_tree_host_factory.h"
#include "mash/wm/background_layout.h"
#include "mash/wm/shelf_layout.h"
#include "mash/wm/window_layout.h"
#include "mash/wm/window_manager_impl.h"
#include "mojo/application/public/cpp/application_connection.h"
#include "mojo/services/tracing/public/cpp/tracing_impl.h"
#include "ui/mojo/init/ui_init.h"
#include "ui/views/mus/aura_init.h"
#include "ui/views/mus/display_converter.h"

namespace mash {
namespace wm {
namespace {
const uint32_t kWindowSwitchCmd = 1;
}  // namespace

WindowManagerApplication::WindowManagerApplication()
    : root_(nullptr),
      window_count_(0),
      app_(nullptr),
      host_client_binding_(this) {}

WindowManagerApplication::~WindowManagerApplication() {}

mus::Window* WindowManagerApplication::GetWindowForContainer(
    mojom::Container container) {
  const mus::Id window_id = root_->connection()->GetConnectionId() << 16 |
                            static_cast<uint16_t>(container);
  return root_->GetChildById(window_id);
}

mus::Window* WindowManagerApplication::GetWindowById(mus::Id id) {
  return root_->GetChildById(id);
}

void WindowManagerApplication::AddAccelerators() {
  host_->AddAccelerator(
      kWindowSwitchCmd,
      mus::CreateKeyMatcher(mus::mojom::KEYBOARD_CODE_TAB,
                            mus::mojom::EVENT_FLAGS_CONTROL_DOWN));
}

void WindowManagerApplication::Initialize(mojo::ApplicationImpl* app) {
  app_ = app;
  tracing_.Initialize(app);
  mus::mojom::WindowManagerPtr window_manager;
  requests_.push_back(new mojo::InterfaceRequest<mus::mojom::WindowManager>(
      mojo::GetProxy(&window_manager)));
  mus::mojom::WindowTreeHostClientPtr host_client;
  host_client_binding_.Bind(GetProxy(&host_client));
  mus::CreateSingleWindowTreeHost(app, host_client.Pass(), this, &host_,
                                  window_manager.Pass(), this);
}

bool WindowManagerApplication::ConfigureIncomingConnection(
    mojo::ApplicationConnection* connection) {
  connection->AddService(this);
  return true;
}

void WindowManagerApplication::OnAccelerator(uint32_t id,
                                             mus::mojom::EventPtr event) {
  switch (id) {
    case kWindowSwitchCmd:
      host_->ActivateNextWindow();
      break;
    default:
      NOTREACHED() << "Unknown accelerator command: " << id;
  }
}

void WindowManagerApplication::OnEmbed(mus::Window* root) {
  root_ = root;
  root_->AddObserver(this);
  CreateContainers();
  background_layout_.reset(new BackgroundLayout(
      GetWindowForContainer(mojom::CONTAINER_USER_BACKGROUND)));
  shelf_layout_.reset(
      new ShelfLayout(GetWindowForContainer(mojom::CONTAINER_USER_SHELF)));

  mus::Window* window = GetWindowForContainer(mojom::CONTAINER_USER_WINDOWS);
  window_layout_.reset(
      new WindowLayout(GetWindowForContainer(mojom::CONTAINER_USER_WINDOWS)));
  host_->AddActivationParent(window->id());

  AddAccelerators();

  window_manager_.reset(new WindowManagerImpl(this));

  ui_init_.reset(new ui::mojo::UIInit(views::GetDisplaysFromWindow(root)));
  aura_init_.reset(new views::AuraInit(app_, "mash_wm_resources.pak"));

  for (auto request : requests_)
    window_manager_binding_.AddBinding(window_manager_.get(), request->Pass());
  requests_.clear();
}

void WindowManagerApplication::OnConnectionLost(
    mus::WindowTreeConnection* connection) {
  // TODO(sky): shutdown.
  NOTIMPLEMENTED();
}

void WindowManagerApplication::Create(
    mojo::ApplicationConnection* connection,
    mojo::InterfaceRequest<mus::mojom::WindowManager> request) {
  if (root_) {
    window_manager_binding_.AddBinding(window_manager_.get(), request.Pass());
  } else {
    requests_.push_back(
        new mojo::InterfaceRequest<mus::mojom::WindowManager>(request.Pass()));
  }
}

void WindowManagerApplication::OnWindowDestroyed(mus::Window* window) {
  DCHECK_EQ(window, root_);
  root_->RemoveObserver(this);
  // Delete the |window_manager_| here so that WindowManager doesn't have to
  // worry about the possibility of |root_| being null.
  window_manager_.reset();
  root_ = nullptr;
}

bool WindowManagerApplication::OnWmSetBounds(mus::Window* window,
                                             gfx::Rect* bounds) {
  // By returning true the bounds of |window| is updated.
  return true;
}

bool WindowManagerApplication::OnWmSetProperty(
    mus::Window* window,
    const std::string& name,
    scoped_ptr<std::vector<uint8_t>>* new_data) {
  // TODO(sky): constrain this to set of keys we know about, and allowed
  // values.
  return name == mus::mojom::WindowManager::kShowState_Property ||
      name == mus::mojom::WindowManager::kPreferredSize_Property ||
      name == mus::mojom::WindowManager::kResizeBehavior_Property;
}

void WindowManagerApplication::CreateContainers() {
  for (uint16_t container =
           static_cast<uint16_t>(mojom::CONTAINER_ALL_USER_BACKGROUND);
       container < static_cast<uint16_t>(mojom::CONTAINER_COUNT); ++container) {
    mus::Window* window = root_->connection()->NewWindow();
    DCHECK_EQ(mus::LoWord(window->id()), container)
        << "Containers must be created before other windows!";
    window->SetBounds(root_->bounds());
    window->SetVisible(true);
    root_->AddChild(window);
  }
}

}  // namespace wm
}  // namespace mash
