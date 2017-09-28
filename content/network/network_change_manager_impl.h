// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_NETWORK_NETWORK_CHANGE_MANAGER_IMPL_H_
#define CONTENT_NETWORK_NETWORK_CHANGE_MANAGER_IMPL_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "content/common/content_export.h"
#include "content/public/common/network_change_manager.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "net/base/network_change_notifier.h"
#include "services/service_manager/public/cpp/binder_registry.h"

namespace net {
class NetworkChangeNotifier;
}  // namespace net

namespace content {

class CONTENT_EXPORT NetworkChangeManagerImpl
    : public mojom::NetworkChangeManager,
      public net::NetworkChangeNotifier::NetworkChangeObserver {
 public:
  NetworkChangeManagerImpl(service_manager::BinderRegistry* registry);

  ~NetworkChangeManagerImpl() override;

  // Binds a NetworkChangeManager request to this object. Mojo messages
  // coming through the associated pipe will be served by this object.
  void AddRequest(mojom::NetworkChangeManagerRequest request);

  void RequestNotifications(
      mojom::NetworkChangeManagerClientPtr client_ptr) override;

  const std::vector<mojom::NetworkChangeManagerClientPtr>&
  GetClientsForTesting() const {
    return clients_;
  }

 private:
  // Handles connection errors on notification pipes.
  void NotificationPipeBroken(mojom::NetworkChangeManagerClient* client);

  // net::NetworkChangeNotifier::NetworkChangeObserver implementation:
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType type) override;

  std::unique_ptr<net::NetworkChangeNotifier> network_change_notifier_;
  mojo::BindingSet<mojom::NetworkChangeManager> bindings_;
  std::vector<mojom::NetworkChangeManagerClientPtr> clients_;

  DISALLOW_COPY_AND_ASSIGN(NetworkChangeManagerImpl);
};

}  // namespace content

#endif  // CONTENT_NETWORK_NETWORK_CHANGE_MANAGER_IMPL_H_
