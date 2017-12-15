// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRONET_NATIVE_ENGINE_H_
#define COMPONENTS_CRONET_NATIVE_ENGINE_H_

#include <memory>

#include "base/macros.h"
#include "components/cronet/native/generated/cronet.idl_impl_interface.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"

namespace cronet {

// Implementation of Cronet_Engine that invokes callback on destroy.
class Cronet_EngineImpl : public Cronet_Engine {
 public:
  Cronet_EngineImpl();
  ~Cronet_EngineImpl() override;

  void SetContext(Cronet_EngineContext context) override;
  Cronet_EngineContext GetContext() override;

  void StartWithParams(Cronet_EngineParamsPtr params) override;
  void StartNetLogToFile(CharString fileName, bool logAll) override;
  void StopNetLog() override;
  CharString GetVersionString() override;
  CharString GetDefaultUserAgent() override;
  void Shutdown() override;

 private:
  std::unique_ptr<net::URLRequestContext> main_context_;
  scoped_refptr<net::URLRequestContextGetter> main_context_getter_;

  // Engine context.
  Cronet_EngineContext context_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(Cronet_EngineImpl);
};

};  // namespace cronet

#endif  // COMPONENTS_CRONET_NATIVE_ENGINE_H_
