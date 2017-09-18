// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_NATIVE_RENDERER_MESSAGING_SERVICE_H_
#define EXTENSIONS_RENDERER_NATIVE_RENDERER_MESSAGING_SERVICE_H_

#include <string>

#include "base/macros.h"
#include "extensions/renderer/gin_port.h"
#include "extensions/renderer/renderer_messaging_service.h"
#include "gin/handle.h"

struct ExtensionMsg_ExternalConnectionInfo;
struct ExtensionMsg_TabConnectionInfo;

namespace extensions {
class NativeExtensionBindingsSystem;
struct Message;
struct PortId;

// The messaging service to handle dispatching extension messages and connection
// events to different contexts.
class NativeRendererMessagingService : public RendererMessagingService,
                                       public GinPort::Delegate {
 public:
  explicit NativeRendererMessagingService(
      NativeExtensionBindingsSystem* bindings_system);
  ~NativeRendererMessagingService() override;

  // Creates and opens a new message port in the specified context.
  gin::Handle<GinPort> Connect(ScriptContext* script_context,
                               const std::string& target_id,
                               const std::string& name,
                               bool include_tls_channel_id);

  // GinPort::Delegate:
  void PostMessageToPort(v8::Local<v8::Context> context,
                         const PortId& port_id,
                         int routing_id,
                         std::unique_ptr<Message> message) override;
  void ClosePort(v8::Local<v8::Context> context,
                 const PortId& port_id,
                 int routing_id) override;

  gin::Handle<GinPort> CreatePortForTesting(ScriptContext* script_context,
                                            const std::string& channel_name,
                                            const PortId& port_id);
  gin::Handle<GinPort> GetPortForTesting(ScriptContext* script_context,
                                         const PortId& port_id);
  bool HasPortForTesting(ScriptContext* script_context, const PortId& port_id);

 private:
  // RendererMessagingService:
  bool ContextHasMessagePort(ScriptContext* script_context,
                             const PortId& port_id) override;
  void DispatchOnConnectToListeners(
      ScriptContext* script_context,
      const PortId& target_port_id,
      const std::string& target_extension_id,
      const std::string& channel_name,
      const ExtensionMsg_TabConnectionInfo* source,
      const ExtensionMsg_ExternalConnectionInfo& info,
      const std::string& tls_channel_id,
      const std::string& event_name) override;
  void DispatchOnMessageToListeners(ScriptContext* script_context,
                                    const Message& message,
                                    const PortId& target_port_id) override;
  void DispatchOnDisconnectToListeners(ScriptContext* script_context,
                                       const PortId& port_id,
                                       const std::string& error) override;

  // Creates a new port in the given context, with the specified |channel_name|
  // and |port_id|. Assumes no such port exists.
  gin::Handle<GinPort> CreatePort(ScriptContext* script_context,
                                  const std::string& channel_name,
                                  const PortId& port_id);

  // Returns the port with the given |port_id| in the given |script_context|;
  // requires that such a port exists.
  gin::Handle<GinPort> GetPort(ScriptContext* script_context,
                               const PortId& port_id);

  // The associated bindings system; guaranteed to outlive this object.
  NativeExtensionBindingsSystem* const bindings_system_;

  DISALLOW_COPY_AND_ASSIGN(NativeRendererMessagingService);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_NATIVE_RENDERER_MESSAGING_SERVICE_H_
