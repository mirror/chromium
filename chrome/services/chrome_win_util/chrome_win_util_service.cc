// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/services/chrome_win_util/chrome_win_util_service.h"

#include <memory>

#include "build/build_config.h"
#include "chrome/services/chrome_win_util/public/interfaces/win_shell_util.mojom.h"
#include "chrome/services/chrome_win_util/win_shell_util_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace chrome {

namespace {

void OnWinShellUtilRequest(
    service_manager::ServiceContextRefFactory* ref_factory,
    chrome::mojom::WinShellUtilRequest request) {
  mojo::MakeStrongBinding(
      std::make_unique<WinShellUtilImpl>(ref_factory->CreateRef()),
      std::move(request));
}

}  // namespace

ChromeWinUtilService::ChromeWinUtilService() = default;

ChromeWinUtilService::~ChromeWinUtilService() = default;

std::unique_ptr<service_manager::Service>
ChromeWinUtilService::CreateService() {
  return std::make_unique<ChromeWinUtilService>();
}

void ChromeWinUtilService::OnStart() {
  ref_factory_.reset(new service_manager::ServiceContextRefFactory(
      base::Bind(&service_manager::ServiceContext::RequestQuit,
                 base::Unretained(context()))));
  registry_.AddInterface(
      base::Bind(&OnWinShellUtilRequest, ref_factory_.get()));
}

void ChromeWinUtilService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

}  //  namespace chrome
