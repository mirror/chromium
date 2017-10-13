// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/services/chrome_file_util/chrome_file_util_service.h"

#include <memory>

#include "build/build_config.h"
#include "chrome/services/chrome_file_util/zip_file_creator_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace chrome {

namespace {

void OnZipFileCreatorRequest(
    service_manager::ServiceContextRefFactory* ref_factory,
    chrome::mojom::ZipFileCreatorRequest request) {
  mojo::MakeStrongBinding(
      std::make_unique<ZipFileCreatorImpl>(ref_factory->CreateRef()),
      std::move(request));
}

}  // namespace

ChromeFileUtilService::ChromeFileUtilService() = default;

ChromeFileUtilService::~ChromeFileUtilService() = default;

std::unique_ptr<service_manager::Service>
ChromeFileUtilService::CreateService() {
  return std::make_unique<ChromeFileUtilService>();
}

void ChromeFileUtilService::OnStart() {
  ref_factory_.reset(new service_manager::ServiceContextRefFactory(
      base::Bind(&service_manager::ServiceContext::RequestQuit,
                 base::Unretained(context()))));
  registry_.AddInterface(
      base::Bind(&OnZipFileCreatorRequest, ref_factory_.get()));
}

void ChromeFileUtilService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

}  //  namespace chrome
