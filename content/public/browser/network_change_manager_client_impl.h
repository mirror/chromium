// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_NETWORK_CHANGE_MANAGER_CLIENT_IMPL_H_
#define CONTENT_PUBLIC_BROWSER_NETWORK_CHANGE_MANAGER_CLIENT_IMPL_H_

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
// NetworkChangeObservers registered through AddObserver()/RemoveObserver().
// This class also exposes static utility methods and synchronous APIs to get
// cached network states.
class CONTENT_EXPORT NetworkChangeManagerClientImpl
    : public mojom::NetworkChangeManagerClient {
 public:
  class CONTENT_EXPORT NetworkChangeObserver {
   public:
    // OnNetworkChanged will be called when a change occurs to the host
    // computer's hardware or software that affects the route network packets
    // take to any network server. Some examples:
    //   1. A network connection becoming available or going away. For example
    //      plugging or unplugging an Ethernet cable, WiFi or cellular modem
    //      connecting or disconnecting from a network, or a VPN tunnel being
    //      established or taken down.
    //   2. An active network connection's IP address changes.
    //   3. A change to the local IP routing tables.
    // The signal shall only be produced when the change is complete.  For
    // example if a new network connection has become available, only give the
    // signal once we think the O/S has finished establishing the connection
    // (i.e. DHCP is done) to the point where the new connection is usable.
    // The signal shall not be produced spuriously as it will be triggering some
    // expensive operations, like socket pools closing all connections and
    // sockets and then re-establishing them.
    // |type| indicates the type of the active primary network connection after
    // the change.  Observers performing "constructive" activities like trying
    // to establish a connection to a server should only do so when
    // |type != CONNECTION_NONE|.  Observers performing "destructive" activities
    // like resetting already established server connections should only do so
    // when |type == CONNECTION_NONE|.  OnNetworkChanged will always be called
    // with CONNECTION_NONE immediately prior to being called with an online
    // state; this is done to make sure that destructive actions take place
    // prior to constructive actions.
    virtual void OnNetworkChanged(mojom::ConnectionType type) = 0;

   protected:
    NetworkChangeObserver() {}
    virtual ~NetworkChangeObserver() {}

   private:
    DISALLOW_COPY_AND_ASSIGN(NetworkChangeObserver);
  };

  explicit NetworkChangeManagerClientImpl(
      mojom::NetworkService* network_service);

  ~NetworkChangeManagerClientImpl() override;

  // Returns the connection type.
  // A return value of |CONNECTION_NONE| is a pretty strong indicator that the
  // user won't be able to connect to remote sites. However, another return
  // value doesn't imply that the user will be able to connect to remote sites;
  // even if some link is up, it is uncertain whether a particular connection
  // attempt to a particular remote site will be successful.
  // The returned value only describes the first-hop connection, for example if
  // the device is connected via WiFi to a 4G hotspot, the returned value will
  // be CONNECTION_WIFI, not CONNECTION_4G.
  mojom::ConnectionType GetConnectionType();

  // Returns the device's current default active network connection's subtype.
  // The returned value only describes the first-hop connection, for example if
  // the device is connected via WiFi to a 4G hotspot, the returned value will
  // reflect WiFi, not 4G. This method may return SUBTYPE_UNKNOWN even if the
  // connection type is known.
  mojom::ConnectionSubtype GetConnectionSubtype();

  // Returns true if |type| is a cellular connection.
  // Returns false if |type| is CONNECTION_UNKNOWN, and thus, depending on the
  // implementation of GetConnectionType(), it is possible that
  // IsConnectionCellular(GetConnectionType()) returns false even if the
  // current connection is cellular.
  static bool IsConnectionCellular(mojom::ConnectionType type);

  // Registers |observer| to receive notifications of network changes. The
  // thread on which this is called is the thread on which |observer| will be
  // called back with notifications.
  void AddNetworkChangeObserver(NetworkChangeObserver* observer);

  // Unregisters |observer| from receiving notifications.  This must be called
  // on the same thread on which AddObserver() was called.
  void RemoveNetworkChangeObserver(NetworkChangeObserver* observer);

 private:
  // NetworkChangeManagerClient implementation:
  void OnNetworkChanged(mojom::ConnectionType type,
                        mojom::ConnectionSubtype subtype) override;

  mojom::ConnectionType connection_type_;
  mojom::ConnectionSubtype connection_subtype_;

  const scoped_refptr<base::ObserverListThreadSafe<NetworkChangeObserver>>
      network_change_observer_list_;

  mojom::NetworkChangeManagerPtr manager_ptr_;

  mojo::Binding<mojom::NetworkChangeManagerClient> binding_;

  DISALLOW_COPY_AND_ASSIGN(NetworkChangeManagerClientImpl);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_NETWORK_CHANGE_MANAGER_CLIENT_IMPL_H_
