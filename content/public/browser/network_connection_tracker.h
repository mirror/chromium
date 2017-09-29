// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_NETWORK_CONNECTION_TRACKER_H_
#define CONTENT_PUBLIC_BROWSER_NETWORK_CONNECTION_TRACKER_H_

#include <memory>

#include "base/macros.h"
#include "base/observer_list_threadsafe.h"
#include "content/common/content_export.h"
#include "content/public/common/network_change_manager.mojom.h"
#include "content/public/common/network_service.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/binding_set.h"

namespace content {

// This class subscribes to network change events from
// mojom::NetworkChangeManager and propogates these notifications to its
// NetworkConnectionObservers registered through AddObserver()/RemoveObserver().
class CONTENT_EXPORT NetworkConnectionTracker
    : public mojom::NetworkChangeManagerClient {
 public:
  class CONTENT_EXPORT NetworkConnectionObserver {
   public:
    // Please refer to net::NetworkChangeNotifier::NetworkChangeObserver for
    // documentation.
    virtual void OnConnectionChanged(mojom::ConnectionType type) = 0;

   protected:
    NetworkConnectionObserver() {}
    virtual ~NetworkConnectionObserver() {}

   private:
    DISALLOW_COPY_AND_ASSIGN(NetworkConnectionObserver);
  };

  explicit NetworkConnectionTracker(mojom::NetworkService* network_service);

  ~NetworkConnectionTracker() override;

  // Please refer to net::NetworkChangeNotifier::GetConnectionType() for
  // documentation. This method is not thread-safe. If used on a different
  // thread, this method might return a stale value.
  mojom::ConnectionType GetConnectionType() const;

  // Please refer to net::NetworkChangeNotifier::GetConnectionSubtype() for
  // documentation. This method is not thread-safe. If used on a different
  // thread, this method might return a stale value.
  mojom::ConnectionSubtype GetConnectionSubtype() const;

  // Please refer to net::NetworkChangeNotifier::IsConnectionCellular() for
  // documentation.
  static bool IsConnectionCellular(mojom::ConnectionType type);

  // Registers |observer| to receive notifications of network changes. The
  // thread on which this is called is the thread on which |observer| will be
  // called back with notifications.
  void AddNetworkConnectionObserver(NetworkConnectionObserver* observer);

  // Unregisters |observer| from receiving notifications.  This must be called
  // on the same thread on which AddObserver() was called.
  // All observers must be unregistred before |this| is destroyed.
  void RemoveNetworkConnectionObserver(NetworkConnectionObserver* observer);

 private:
  // NetworkChangeManagerClient implementation:
  void OnNetworkChanged(mojom::ConnectionType type,
                        mojom::ConnectionSubtype subtype) override;

  mojom::ConnectionType connection_type_;
  mojom::ConnectionSubtype connection_subtype_;

  const scoped_refptr<base::ObserverListThreadSafe<NetworkConnectionObserver>>
      network_change_observer_list_;

  mojo::Binding<mojom::NetworkChangeManagerClient> binding_;

  DISALLOW_COPY_AND_ASSIGN(NetworkConnectionTracker);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_NETWORK_CONNECTION_TRACKER_H_
