// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRONET_NATIVE_ENGINE_H_
#define COMPONENTS_CRONET_NATIVE_ENGINE_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"
#include "components/cronet/cronet_url_request_context.h"
#include "components/cronet/native/generated/cronet.idl_impl_interface.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"

namespace cronet {

// Implementation of Cronet_Engine that uses CronetURLRequestContext.
class Cronet_EngineImpl : public Cronet_Engine {
 public:
  Cronet_EngineImpl();
  ~Cronet_EngineImpl() override;

  // Cronet_Engine implementation:
  void SetContext(Cronet_EngineContext context) override;
  Cronet_EngineContext GetContext() override;
  void StartWithParams(Cronet_EngineParamsPtr params) override;
  bool StartNetLogToFile(CharString fileName, bool logAll) override;
  void StopNetLog() override;
  CharString GetVersionString() override;
  CharString GetDefaultUserAgent() override;
  void Shutdown() override;

 private:
  class Callback;

  std::unique_ptr<CronetURLRequestContext> context_;
  // Synchronize access to |context_| from different threads.
  base::Lock context_lock_;
  // Signaled when |context_| initialization is done.
  base::WaitableEvent init_completed_;

  // Flag that indicates whether logging is in progress.
  bool is_logging_ = false;
  // Signaled when |StopNetLog| is done.
  std::unique_ptr<base::WaitableEvent> stop_netlog_completed_;

  // Storage path used by this engine.
  std::string in_use_storage_path_;

  // Engine context. Not owned, accessed from client thread.
  Cronet_EngineContext engine_context_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(Cronet_EngineImpl);
};

};  // namespace cronet

#endif  // COMPONENTS_CRONET_NATIVE_ENGINE_H_
