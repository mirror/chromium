// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/network/network_change_manager_impl.h"

#include <algorithm>
#include <utility>

#include "base/logging.h"
#include "net/base/network_change_notifier.h"

#if defined(OS_ANDROID)
#include "net/android/network_change_notifier_factory_android.h"
#endif

namespace content {

NetworkChangeManagerImpl::NetworkChangeManagerImpl(
    bool create_network_change_notifier) {
  if (create_network_change_notifier) {
#if defined(OS_ANDROID)
    network_change_notifier_factory_ =
        std::make_unique<net::NetworkChangeNotifierFactoryAndroid>();
    network_change_notifier_.reset(
        network_change_notifier_factory_->CreateInstance());
#elif defined(OS_CHROMEOS)
    NOTIMPLEMENTED();
#else
    network_change_notifier_.reset(net::NetworkChangeNotifier::Create());
#endif
  }
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
  connection_type_ =
      mojom::ConnectionType(net::NetworkChangeNotifier::GetConnectionType());
}

NetworkChangeManagerImpl::~NetworkChangeManagerImpl() {
  net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
}

void NetworkChangeManagerImpl::AddRequest(
    mojom::NetworkChangeManagerRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void NetworkChangeManagerImpl::RequestNotifications(
    mojom::NetworkChangeManagerClientPtr client_ptr) {
  client_ptr.set_connection_error_handler(
      base::Bind(&NetworkChangeManagerImpl::NotificationPipeBroken,
                 // base::Unretained is safe as destruction of the
                 // NetworkChangeManagerImpl will also destroy the
                 // |clients_| list (which this object will be
                 // inserted into, below), which will destroy the
                 // client_ptr, rendering this callback moot.
                 base::Unretained(this), base::Unretained(client_ptr.get())));
  if (connection_type_ != mojom::ConnectionType::CONNECTION_UNKNOWN)
    client_ptr->OnNetworkChanged(connection_type_);
  clients_.push_back(std::move(client_ptr));
}

size_t NetworkChangeManagerImpl::GetNumClientsForTesting() const {
  return clients_.size();
}

void NetworkChangeManagerImpl::NotificationPipeBroken(
    mojom::NetworkChangeManagerClient* client) {
  clients_.erase(
      std::find_if(clients_.begin(), clients_.end(),
                   [client](mojom::NetworkChangeManagerClientPtr& ptr) {
                     return ptr.get() == client;
                   }));
}

void NetworkChangeManagerImpl::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  connection_type_ = mojom::ConnectionType(type);
  for (const auto& client : clients_) {
    client->OnNetworkChanged(connection_type_);
  }
}

}  // namespace content
