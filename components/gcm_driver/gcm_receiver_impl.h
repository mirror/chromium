// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_GCM_DRIVER_MOJO_GCM_RECEIVER_IMPL_H_
#define COMPONENTS_GCM_DRIVER_MOJO_GCM_RECEIVER_IMPL_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "base/macros.h"
#include "components/gcm_driver/public/interfaces/gcm_receiver.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace gcm {

class GCMService;
class GCMAppHandler;

// Wrap it with GCMConnectionObserver
class GCMReceiverImpl : public mojom::GCMReceiver {
 public:
  explicit GCMReceiverImpl(GCMService* service);
  ~GCMReceiverImpl() override;

  void AddBinding(mojo::InterfaceRequest<mojom::GCMReceiver> request);

  void AddGCMHandler(const std::string& app_id,
                     mojom::GCMHandlerPtr handler) override;

  void RemoveGCMHandler(const std::string& app_id) override;

  void GetGCMHandler(const std::string& app_id,
                     GetGCMHandlerCallback callback) override;

  void Register(const std::string& app_id,
                const std::vector<std::string>& sender_ids,
                RegisterCallback callback) override;
  void OnRegistrationCompletion(RegisterCallback callback,
                                const std::string& iid_token,
                                GCMClient::Result result);

 private:
  GCMService* service_;
  std::unordered_map<std::string, std::unique_ptr<GCMAppHandler>> app_handlers_;
  mojo::Binding<mojom::GCMReceiver> binding_;
  DISALLOW_COPY_AND_ASSIGN(GCMReceiverImpl);
};

}  // namespace gcm

#endif  // COMPONENTS_GCM_DRIVER_MOJO_GCM_RECEIVER_IMPL_H_
