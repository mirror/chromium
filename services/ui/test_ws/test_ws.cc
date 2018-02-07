// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/service_manager/public/c/main.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_context.h"
#include "services/service_manager/public/cpp/service_runner.h"
#include "services/ui/public/interfaces/window_tree_host_factory.mojom.h"
#include "services/ui/ws2/window_service.h"
#include "services/ui/ws2/window_service_client.h"
#include "services/ui/ws2/window_service_client_binding.h"
#include "services/ui/ws2/window_service_delegate.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/client/default_capture_client.h"
#include "ui/aura/env.h"
#include "ui/aura/mus/property_converter.h"
#include "ui/aura/mus/property_utils.h"
#include "ui/aura/test/aura_test_helper.h"
#include "ui/aura/test/test_focus_client.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/ui_base_paths.h"
#include "ui/compositor/test/context_factories_for_test.h"
#include "ui/display/display.h"
#include "ui/display/display_list.h"
#include "ui/display/screen_base.h"
#include "ui/gfx/gfx_paths.h"
#include "ui/gl/test/gl_surface_test_support.h"
#include "ui/wm/core/capture_controller.h"
#include "ui/wm/core/wm_state.h"

namespace ui {
namespace test {

class TestWS;

class WindowTreeHostFactory : public mojom::WindowTreeHostFactory {
 public:
  explicit WindowTreeHostFactory(TestWS* test_ws) : test_ws_(test_ws) {}

  ~WindowTreeHostFactory() override = default;

  void AddBinding(mojom::WindowTreeHostFactoryRequest request) {
    window_tree_host_factory_bindings_.AddBinding(this, std::move(request));
  }

 private:
  // mojom::WindowTreeHostFactory implementation.
  void CreateWindowTreeHost(mojom::WindowTreeHostRequest host,
                            mojom::WindowTreeClientPtr tree_client) override;

  TestWS* test_ws_;

  mojo::BindingSet<mojom::WindowTreeHostFactory>
      window_tree_host_factory_bindings_;

  DISALLOW_COPY_AND_ASSIGN(WindowTreeHostFactory);
};

class TestWS : public service_manager::Service,
               public ws2::WindowServiceDelegate {
 public:
  TestWS() : window_tree_host_factory_(this) {}

  ~TestWS() override {
    // AuraTestHelper expects TearDown() to be called.
    aura_test_helper_->TearDown();
    aura_test_helper_.reset();
    ui::TerminateContextFactoryForTests();
  }

  void AddNewClient(mojom::WindowTreeHostRequest host,
                    mojom::WindowTreeClientPtr tree_client) {
    // XXX leaks!
    aura::Window* window = new aura::Window(nullptr);
    window->Init(LAYER_NOT_DRAWN);
    aura_test_helper_->root_window()->AddChild(window);
    const bool intercepts_events = false;
    auto binding = std::make_unique<ws2::WindowServiceClientBinding>(
        window_service_.get(), std::move(tree_client), intercepts_events,
        window);
    window_service_client_bindings_.push_back(std::move(binding));
  }

 private:
  void BindWindowTreeHostFactoryRequest(
      mojom::WindowTreeHostFactoryRequest request) {
    window_tree_host_factory_.AddBinding(std::move(request));
  }

  // WindowServiceDelegate:
  void ClientRequestedWindowBoundsChange(aura::Window* window,
                                         const gfx::Rect& bounds) override {
    window->SetBounds(bounds);
  }
  std::unique_ptr<aura::Window> NewTopLevel(
      const std::unordered_map<std::string, std::vector<uint8_t>>& properties)
      override {
    std::unique_ptr<aura::Window> top_level =
        std::make_unique<aura::Window>(nullptr);
    top_level->Init(LAYER_NOT_DRAWN);
    return top_level;
  }

  // service_manager::Service:
  void OnStart() override {
    CHECK(!started_);
    started_ = true;

    gl::GLSurfaceTestSupport::InitializeOneOff();
    gfx::RegisterPathProvider();
    ui::RegisterPathProvider();

    window_service_ = std::make_unique<ws2::WindowService>(this);

    ui::ContextFactory* context_factory = nullptr;
    ui::ContextFactoryPrivate* context_factory_private = nullptr;
    ui::InitializeContextFactoryForTests(false /* enable_pixel_output */,
                                         &context_factory,
                                         &context_factory_private);
    aura_test_helper_ = std::make_unique<aura::test::AuraTestHelper>();
    aura_test_helper_->SetUp(context_factory, context_factory_private);

    registry_.AddInterface<mojom::WindowTreeHostFactory>(base::Bind(
        &TestWS::BindWindowTreeHostFactoryRequest, base::Unretained(this)));
  }
  void OnBindInterface(const service_manager::BindSourceInfo& source_info,
                       const std::string& interface_name,
                       mojo::ScopedMessagePipeHandle interface_pipe) override {
    registry_.BindInterface(interface_name, std::move(interface_pipe));
  }

  service_manager::BinderRegistry registry_;

  std::unique_ptr<aura::test::AuraTestHelper> aura_test_helper_;
  WindowTreeHostFactory window_tree_host_factory_;
  aura::PropertyConverter property_converter_;
  std::unique_ptr<ws2::WindowService> window_service_;

  bool started_ = false;

  std::unique_ptr<ws2::WindowServiceClientBinding>
      window_service_client_binding_;
  std::vector<std::unique_ptr<ws2::WindowServiceClientBinding>>
      window_service_client_bindings_;
  DISALLOW_COPY_AND_ASSIGN(TestWS);
};

void WindowTreeHostFactory::CreateWindowTreeHost(
    mojom::WindowTreeHostRequest host,
    mojom::WindowTreeClientPtr tree_client) {
  test_ws_->AddNewClient(std::move(host), std::move(tree_client));
}

}  // namespace test
}  // namespace ui

MojoResult ServiceMain(MojoHandle service_request_handle) {
  service_manager::ServiceRunner runner(new ui::test::TestWS);
  return runner.Run(service_request_handle);
}
