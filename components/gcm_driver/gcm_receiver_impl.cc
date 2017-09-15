// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/gcm_driver/gcm_receiver_impl.h"
#include "components/gcm_driver/gcm_app_handler.h"
#include "components/gcm_driver/gcm_connection_observer.h"
#include "components/gcm_driver/gcm_driver.h"
#include "components/gcm_driver/gcm_receiver_typemap_traits.h"
#include "components/gcm_driver/gcm_service.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/memory/ptr_util.h"

namespace gcm {

GCMAppHandlerWrapper::GCMAppHandlerWrapper(mojom::GCMHandlerPtr handler)
    : handler_(std::move(handler)) {}

void GCMAppHandlerWrapper::OnEvent(const std::string& event) {
  handler_->OnEvent(event);
}

void GCMAppHandlerWrapper::OnStoreReset() {
  handler_->OnStoreReset();
}

void GCMAppHandlerWrapper::OnMessage(const std::string& app_id,
                                     const IncomingMessage& message) {
  handler_->OnMessage(app_id, message);
}

void GCMAppHandlerWrapper::OnMessagesDeleted(const std::string& app_id) {
  handler_->OnMessagesDeleted(app_id);
}

void GCMAppHandlerWrapper::OnSendError(
    const std::string& app_id,
    const GCMClient::SendErrorDetails& send_error_details) {
  handler_->OnSendError(app_id, send_error_details);
}

void GCMAppHandlerWrapper::OnSendAcknowledged(const std::string& app_id,
                                              const std::string& message_id) {
  handler_->OnSendAcknowledged(app_id, message_id);
}

void GCMAppHandlerWrapper::ShutdownHandler() {
  handler_->ShutdownHandler();
}

GCMReceiverImpl::GCMReceiverImpl(GCMService* service)
    : service_(service), binding_(this) {
  DCHECK(service_);
  service_->gcm_driver()->AddConnectionObserver(this);
}

GCMReceiverImpl::~GCMReceiverImpl() {
  service_->gcm_driver()->RemoveConnectionObserver(this);
}

void GCMReceiverImpl::AddBinding(
    mojo::InterfaceRequest<mojom::GCMReceiver> request) {
  DCHECK(!binding_.is_bound());
  binding_.Bind(std::move(request));
}

void GCMReceiverImpl::HaveGCMHandler(const std::string& app_id,
                                     HaveGCMHandlerCallback callback) {
  std::move(callback).Run(service_->gcm_driver()->GetAppHandler(app_id) &&
                          app_handlers_.find(app_id) != app_handlers_.end());
}

void GCMReceiverImpl::AddGCMHandler(const std::string& app_id,
                                    mojom::GCMHandlerPtr handler) {
  if (app_handlers_.find(app_id) != app_handlers_.end()) {
    return;
  }
  auto app_handler = base::MakeUnique<GCMAppHandlerWrapper>(std::move(handler));
  service_->gcm_driver()->AddAppHandler(app_id, app_handler.get());
  app_handlers_.emplace(app_id, std::move(app_handler));
}

void GCMReceiverImpl::RemoveGCMHandler(const std::string& app_id) {
  if (app_handlers_.find(app_id) == app_handlers_.end()) {
    return;
  }
  if (service_->gcm_driver()->GetAppHandler(app_id)) {
    service_->gcm_driver()->RemoveAppHandler(app_id);
  }
}

void GCMReceiverImpl::Register(const std::string& app_id,
                               const std::vector<std::string>& sender_ids,
                               RegisterCallback callback) {
  service_->gcm_driver()->Register(
      app_id, sender_ids,
      base::Bind(&GCMReceiverImpl::OnRegistrationCompletion,
                 base::Unretained(this), base::Passed(&callback)));
}

void GCMReceiverImpl::OnRegistrationCompletion(RegisterCallback callback,
                                               const std::string& iid_token,
                                               GCMClient::Result result) {
  std::move(callback).Run(iid_token, result);
}

void GCMReceiverImpl::OnConnected(const net::IPEndPoint& ip_endpoint) {
  for (auto& app : app_handlers_) {
    app.second->OnEvent("Connected");
  }
}

void GCMReceiverImpl::OnDisconnected() {
  for (auto& app : app_handlers_) {
    app.second->OnEvent("Disconnected");
  }
}
}  // namespace gcm
