// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/services/media_util/media_util_service.h"

#include "build/build_config.h"
#include "chrome/services/media_util/media_parser.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace chrome {

namespace {

void OnMediaParserRequest(
    service_manager::ServiceContextRefFactory* ref_factory,
    mojom::MediaParserRequest request) {
  mojo::MakeStrongBinding(
      std::make_unique<MediaParser>(ref_factory->CreateRef()),
      std::move(request));
}

}  // namespace

MediaUtilService::MediaUtilService() = default;

MediaUtilService::~MediaUtilService() = default;

std::unique_ptr<service_manager::Service> MediaUtilService::CreateService() {
  return std::make_unique<MediaUtilService>();
}

void MediaUtilService::OnStart() {
  ref_factory_ = std::make_unique<service_manager::ServiceContextRefFactory>(
      base::Bind(&service_manager::ServiceContext::RequestQuit,
                 base::Unretained(context())));
  registry_.AddInterface(base::Bind(&OnMediaParserRequest, ref_factory_.get()));
}

void MediaUtilService::OnBindInterface(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  registry_.BindInterface(interface_name, std::move(interface_pipe));
}

}  //  namespace chrome
