// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_IPC_MESSAGE_SENDER_H_
#define EXTENSIONS_RENDERER_IPC_MESSAGE_SENDER_H_

#include "base/macros.h"

#include <memory>
#include <string>

#include "extensions/renderer/bindings/api_binding_types.h"

struct ExtensionHostMsg_Request_Params;

namespace extensions {
class ScriptContext;
class WorkerThreadDispatcher;

class IPCMessageSender {
 public:
  virtual ~IPCMessageSender();

  virtual void SendRequestIPC(
      ScriptContext* context,
      std::unique_ptr<ExtensionHostMsg_Request_Params> params,
      binding::RequestThread thread) = 0;
  virtual void SendOnRequestResponseReceivedIPC(int request_id) = 0;

  static std::unique_ptr<IPCMessageSender> CreateMainThreadIPCMessageSender();
  static std::unique_ptr<IPCMessageSender> CreateWorkerThreadIPCMessageSender(
      WorkerThreadDispatcher* dispatcher,
      int64_t service_worker_version_id);

 protected:
  IPCMessageSender();

  DISALLOW_COPY_AND_ASSIGN(IPCMessageSender);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_IPC_MESSAGE_SENDER_H_
