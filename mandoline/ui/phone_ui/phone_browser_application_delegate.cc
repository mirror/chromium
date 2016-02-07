// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mandoline/ui/phone_ui/phone_browser_application_delegate.h"

#include "base/command_line.h"
#include "components/mus/public/cpp/window.h"
#include "components/mus/public/cpp/window_tree_connection.h"
#include "components/mus/public/cpp/window_tree_host_factory.h"
#include "mojo/converters/geometry/geometry_type_converters.h"
#include "mojo/services/network/public/interfaces/url_loader.mojom.h"
#include "mojo/shell/public/cpp/connection.h"
#include "mojo/shell/public/cpp/shell.h"
#include "ui/gfx/geometry/size.h"
#include "url/gurl.h"

namespace mandoline {

////////////////////////////////////////////////////////////////////////////////
// PhoneBrowserApplicationDelegate, public:

PhoneBrowserApplicationDelegate::PhoneBrowserApplicationDelegate()
    : shell_(nullptr),
      root_(nullptr),
      content_(nullptr),
      web_view_(this),
      default_url_("http://www.google.com/") {
}

PhoneBrowserApplicationDelegate::~PhoneBrowserApplicationDelegate() {
  if (root_)
    root_->RemoveObserver(this);
}

////////////////////////////////////////////////////////////////////////////////
// PhoneBrowserApplicationDelegate, mojo::ShellClient implementation:

void PhoneBrowserApplicationDelegate::Initialize(mojo::Shell* shell,
                                                 const std::string& url,
                                                 uint32_t id) {
  shell_ = shell;

  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  for (const auto& arg : command_line->GetArgs()) {
    GURL url(arg);
    if (url.is_valid()) {
      default_url_ = url.spec();
      break;
    }
  }
  mus::CreateWindowTreeHost(shell_, this, &host_, nullptr, nullptr);
}

bool PhoneBrowserApplicationDelegate::AcceptConnection(
    mojo::Connection* connection) {
  connection->AddService<LaunchHandler>(this);
  return true;
}

////////////////////////////////////////////////////////////////////////////////
// PhoneBrowserApplicationDelegate, LaunchHandler implementation:

void PhoneBrowserApplicationDelegate::LaunchURL(const mojo::String& url) {
  mojo::URLRequestPtr request(mojo::URLRequest::New());
  request->url = url;
  web_view_.web_view()->LoadRequest(request.Pass());
}

////////////////////////////////////////////////////////////////////////////////
// PhoneBrowserApplicationDelegate, mus::WindowTreeDelegate implementation:

void PhoneBrowserApplicationDelegate::OnEmbed(mus::Window* root) {
  CHECK(!root_);
  root_ = root;
  content_ = root->connection()->NewWindow();
  root->AddChild(content_);
  content_->SetBounds(root->bounds());
  content_->SetVisible(true);
  root->AddObserver(this);

  host_->SetSize(mojo::Size::From(gfx::Size(320, 640)));
  web_view_.Init(shell_, content_);
  LaunchURL(default_url_);
}

void PhoneBrowserApplicationDelegate::OnConnectionLost(
    mus::WindowTreeConnection* connection) {}

////////////////////////////////////////////////////////////////////////////////
// PhoneBrowserApplicationDelegate, mus::WindowObserver implementation:

void PhoneBrowserApplicationDelegate::OnWindowBoundsChanged(
    mus::Window* view,
    const gfx::Rect& old_bounds,
    const gfx::Rect& new_bounds) {
  CHECK_EQ(view, root_);
  content_->SetBounds(gfx::Rect(new_bounds.size()));
}

////////////////////////////////////////////////////////////////////////////////
// PhoneBrowserApplicationDelegate,
//     web_view::mojom::WebViewClient implementation:

void PhoneBrowserApplicationDelegate::TopLevelNavigateRequest(
    mojo::URLRequestPtr request) {
  web_view_.web_view()->LoadRequest(request.Pass());
}

void PhoneBrowserApplicationDelegate::TopLevelNavigationStarted(
    const mojo::String& url) {}
void PhoneBrowserApplicationDelegate::LoadingStateChanged(bool is_loading,
                                                          double progress) {}

void PhoneBrowserApplicationDelegate::BackForwardChanged(
    web_view::mojom::ButtonState back_button,
    web_view::mojom::ButtonState forward_button) {
  // ...
}

void PhoneBrowserApplicationDelegate::TitleChanged(const mojo::String& title) {
  // ...
}

////////////////////////////////////////////////////////////////////////////////
// PhoneBrowserApplicationDelegate,
//       mojo::InterfaceFactory<LaunchHandler> implementation:

void PhoneBrowserApplicationDelegate::Create(
    mojo::Connection* connection,
    mojo::InterfaceRequest<LaunchHandler> request) {
  launch_handler_bindings_.AddBinding(this, request.Pass());
}

}  // namespace mandoline
