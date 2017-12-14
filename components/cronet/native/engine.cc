// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cronet/native/engine.h"

#include "base/logging.h"
#include "base/macros.h"

namespace cronet {

Cronet_EngineImpl::Cronet_EngineImpl() = default;
Cronet_EngineImpl::~Cronet_EngineImpl() {}

void Cronet_EngineImpl::SetContext(Cronet_EngineContext context) {
  context_ = context;
}

Cronet_EngineContext Cronet_EngineImpl::GetContext() {
  return context_;
}

void Cronet_EngineImpl::StartWithParams(Cronet_EngineParamsPtr params) {}

void Cronet_EngineImpl::StartNetLogToFile(CharString fileName, bool logAll) {}

void Cronet_EngineImpl::StopNetLog() {}

CharString Cronet_EngineImpl::GetVersionString() {
  return "";
}

CharString Cronet_EngineImpl::GetDefaultUserAgent() {
  return "";
}

};  // namespace cronet

CRONET_EXPORT Cronet_EnginePtr Cronet_Engine_Create() {
  return new cronet::Cronet_EngineImpl();
}
