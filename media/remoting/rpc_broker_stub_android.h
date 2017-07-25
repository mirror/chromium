// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_REMOTING_RPC_BROKER_STUB_ANDROID_H_
#define MEDIA_REMOTING_RPC_BROKER_STUB_ANDROID_H_

#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"

namespace pb {
struct RpcMessage {};
}  // namespace pb

namespace media {
namespace remoting {

// Minimum implementation of the RpcBroker to use on platforms where media
// remoting is not fully supported (Android).
class RpcBroker {
 public:
  using SendMessageCallback =
      base::Callback<void(std::unique_ptr<std::vector<uint8_t>>)>;
  explicit RpcBroker(const SendMessageCallback& send_message_cb);
  ~RpcBroker();

  // Get unique handle value (larger than 0) for RPC message handles.
  int GetUniqueHandle() { return kInvalidHandle; }

  using ReceiveMessageCallback =
      base::Callback<void(std::unique_ptr<pb::RpcMessage>)>;
  // Register a component to receive messages via the given
  // ReceiveMessageCallback. |handle| is a unique handle value provided by a
  // prior call to GetUniqueHandle() and is used to reference the component in
  // the RPC messages. The receiver can then use it to direct an RPC message
  // back to a specific component.
  void RegisterMessageReceiverCallback(int handle,
                                       const ReceiveMessageCallback& callback) {
  }

  // Allows components to unregister in order to stop receiving message.
  void UnregisterMessageReceiverCallback(int handle) {}

  // Allows RpcBroker to distribute incoming RPC message to desired components.
  void ProcessMessageFromRemote(std::unique_ptr<pb::RpcMessage> message) {}
  // Sends RPC message to remote end point. The actually sender which sets
  // SendMessageCallback to RpcBrokwer will receive RPC message to do actual
  // data transmission.
  void SendMessageToRemote(std::unique_ptr<pb::RpcMessage> message) {}

  // Gets weak pointer of RpcBroker. This allows callers to post tasks to
  // RpcBroker on the main thread.
  base::WeakPtr<RpcBroker> GetWeakPtr() { return weak_factory_.GetWeakPtr(); }

  // Overwrites |send_message_cb_|. This is used only for test purposes.
  void SetMessageCallbackForTesting(
      const SendMessageCallback& send_message_cb) {}

  // Predefined invalid handle value for RPC message.
  static constexpr int kInvalidHandle = -1;

  // Predefined handle value for RPC messages related to initialization (before
  // the receiver handle(s) are known).
  static constexpr int kAcquireHandle = 0;

  // The first handle to return from GetUniqueHandle().
  static constexpr int kFirstHandle = 1;

 private:
  base::WeakPtrFactory<RpcBroker> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(RpcBroker);
};

}  // namespace remoting
}  // namespace media

#endif  // MEDIA_REMOTING_RPC_BROKER_STUB_ANDROID_H_
