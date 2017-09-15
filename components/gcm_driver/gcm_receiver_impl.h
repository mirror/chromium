// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_GCM_DRIVER_MOJO_GCM_RECEIVER_IMPL_H_
#define COMPONENTS_GCM_DRIVER_MOJO_GCM_RECEIVER_IMPL_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "base/macros.h"
#include "components/gcm_driver/gcm_app_handler.h"
#include "components/gcm_driver/gcm_connection_observer.h"
#include "components/gcm_driver/public/interfaces/gcm_receiver.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"

namespace gcm {

class GCMService;

class GCMAppHandlerWrapper : public GCMAppHandler {
 public:
  GCMAppHandlerWrapper(mojom::GCMHandlerPtr handler);
  ~GCMAppHandlerWrapper() override;
  void OnEvent(const std::string& event);

  // GCMAppHandler implementation.
  void OnStoreReset() override;
  void OnMessage(const std::string& app_id,
                 const IncomingMessage& message) override;
  void OnMessagesDeleted(const std::string& app_id) override;
  void OnSendError(
      const std::string& app_id,
      const GCMClient::SendErrorDetails& send_error_details) override;
  void OnSendAcknowledged(const std::string& app_id,
                          const std::string& message_id) override;
  void ShutdownHandler() override;

 private:
  mojom::GCMHandlerPtr handler_;
  DISALLOW_COPY_AND_ASSIGN(GCMAppHandlerWrapper);
};

// Wrap it with GCMConnectionObserver
class GCMReceiverImpl : public mojom::GCMReceiver,
                        public GCMConnectionObserver {
 public:
  explicit GCMReceiverImpl(GCMService* service);
  ~GCMReceiverImpl() override;

  void AddBinding(mojo::InterfaceRequest<mojom::GCMReceiver> request);

  void HaveGCMHandler(const std::string& app_id,
                      HaveGCMHandlerCallback callback) override;

  void AddGCMHandler(const std::string& app_id,
                     mojom::GCMHandlerPtr handler) override;

  void RemoveGCMHandler(const std::string& app_id) override;

  void Register(const std::string& app_id,
                const std::vector<std::string>& sender_ids,
                RegisterCallback callback) override;
  void OnRegistrationCompletion(RegisterCallback callback,
                                const std::string& iid_token,
                                GCMClient::Result result);

  // GCMConnectionObserver implementation.
  void OnConnected(const net::IPEndPoint& ip_endpoint) override;
  void OnDisconnected() override;

 private:
  GCMService* service_;
  std::unordered_map<std::string, std::unique_ptr<GCMAppHandlerWrapper>>
      app_handlers_;
  mojo::Binding<mojom::GCMReceiver> binding_;
  DISALLOW_COPY_AND_ASSIGN(GCMReceiverImpl);
};

}  // namespace gcm

#endif  // COMPONENTS_GCM_DRIVER_MOJO_GCM_RECEIVER_IMPL_H_
