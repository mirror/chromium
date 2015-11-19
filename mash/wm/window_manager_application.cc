// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mash/wm/window_manager_application.h"

#include "components/mus/common/util.h"
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

WindowManagerApplication::WindowManagerApplication()
    : root_(nullptr), window_count_(0), app_(nullptr) {}
WindowManagerApplication::~WindowManagerApplication() {}

mus::Window* WindowManagerApplication::GetWindowForContainer(
    mash::wm::mojom::Container container) {
  const mus::Id window_id = root_->connection()->GetConnectionId() << 16 |
                            static_cast<uint16_t>(container);
  return root_->GetChildById(window_id);
}

mus::Window* WindowManagerApplication::GetWindowById(mus::Id id) {
  return root_->GetChildById(id);
}

void WindowManagerApplication::Initialize(mojo::ApplicationImpl* app) {
  app_ = app;
  tracing_.Initialize(app);
  mus::mojom::WindowManagerPtr window_manager;
  requests_.push_back(new mojo::InterfaceRequest<mus::mojom::WindowManager>(
      mojo::GetProxy(&window_manager)));
  mus::CreateSingleWindowTreeHost(app, this, &host_, window_manager.Pass(),
                                  this);
}

bool WindowManagerApplication::ConfigureIncomingConnection(
    mojo::ApplicationConnection* connection) {
  connection->AddService(this);
  return true;
}

void WindowManagerApplication::OnEmbed(mus::Window* root) {
  root_ = root;
  root_->AddObserver(this);
  CreateContainers();
  background_layout_.reset(new BackgroundLayout(
      GetWindowForContainer(mash::wm::mojom::CONTAINER_USER_BACKGROUND)));
  shelf_layout_.reset(new ShelfLayout(
      GetWindowForContainer(mash::wm::mojom::CONTAINER_USER_SHELF)));
  window_layout_.reset(new WindowLayout(
      GetWindowForContainer(mash::wm::mojom::CONTAINER_USER_WINDOWS)));

  window_manager_.reset(new WindowManagerImpl(this));

  ui_init_.reset(new ui::mojo::UIInit(views::GetDisplaysFromWindow(root)));
  aura_init_.reset(new views::AuraInit(app_, "views_mus_resources.pak"));

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
  for (uint16_t container = static_cast<uint16_t>(
           mash::wm::mojom::CONTAINER_ALL_USER_BACKGROUND);
       container < static_cast<uint16_t>(mash::wm::mojom::CONTAINER_COUNT);
       ++container) {
    mus::Window* window = root_->connection()->NewWindow();
    DCHECK_EQ(mus::LoWord(window->id()), container)
        << "Containers must be created before other windows!";
    window->SetBounds(root_->bounds());
    window->SetVisible(true);
    root_->AddChild(window);
  }
}
