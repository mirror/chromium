// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/system_web_apps/system_app_helper.h"

#include <utility>

#include "chrome/browser/chromeos/test_system_app/test_system_app.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/mojo_web_ui_handler.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/bindings_policy.h"
#include "mojo/public/cpp/system/core.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(SystemWebAppHelper);

SystemWebAppMojoHandler::SystemWebAppMojoHandler(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {
  LOG(ERROR) << "BDG: Creating handler " << this << " for " << web_contents;
  registry_.AddInterface<mojom::TestSystemApp>(base::Bind(
      &SystemWebAppMojoHandler::BindTestSystemApp, base::Unretained(this)));
}

SystemWebAppMojoHandler::~SystemWebAppMojoHandler() {}

void SystemWebAppMojoHandler::OnInterfaceRequestFromFrame(
    content::RenderFrameHost* render_frame_host,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle* interface_pipe) {
  LOG(ERROR) << "BDG: Interface requested " << interface_name;
  registry_.TryBindInterface(interface_name, interface_pipe);
}

void SystemWebAppMojoHandler::RenderFrameCreated(
    content::RenderFrameHost* render_frame_host) {
  LOG(ERROR) << "BDG: Updating bindings for " << render_frame_host;
  //render_frame_host->AllowBindings(content::BINDINGS_POLICY_WEB_UI);
}

void SystemWebAppMojoHandler::BindTestSystemApp(
    mojo::InterfaceRequest<mojom::TestSystemApp> request) {
  test_system_app_.reset(new TestSystemAppImpl(std::move(request)));
}

SystemWebAppHelper::~SystemWebAppHelper() {}

void SystemWebAppHelper::RenderViewCreated(
    content::RenderViewHost* render_view_host) {
  mojo_handler_.reset(new SystemWebAppMojoHandler(web_contents()));
}

void SystemWebAppHelper::RenderViewHostChanged(
    content::RenderViewHost* old_host,
    content::RenderViewHost* new_host) {
  LOG(ERROR) << "BDG: CHANGING IT";
  //mojo_handler_.reset();
}

SystemWebAppHelper::SystemWebAppHelper(content::WebContents* contents)
    : content::WebContentsObserver(contents) {}
