// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_NETWORK_NETWORK_CHANGE_MANAGER_CLIENT_IMPL_H_
#define CONTENT_NETWORK_NETWORK_CHANGE_MANAGER_CLIENT_IMPL_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/observer_list.h"
#include "components/network_change_manager_client/network_change_manager_client_export.h"
#include "content/public/common/network_change_manager.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/binding_set.h"

namespace network_change_manager_client {

class NETWORK_CHANGE_MANAGER_CLIENT_EXPORT NetworkChangeManagerClientImpl
    : public content::mojom::NetworkChangeManagerClient {
 public:
  class NetworkChangeObserver {
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
    virtual void OnNetworkChanged(content::mojom::ConnectionType type) = 0;

   protected:
    NetworkChangeObserver() {}
    virtual ~NetworkChangeObserver() {}

   private:
    DISALLOW_COPY_AND_ASSIGN(NetworkChangeObserver);
  };

  explicit NetworkChangeManagerClientImpl();

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
  static NETWORK_CHANGE_MANAGER_CLIENT_EXPORT content::mojom::ConnectionType
  GetConnectionType();

  // Returns the device's current default active network connection's subtype.
  // The returned value only describes the first-hop connection, for example if
  // the device is connected via WiFi to a 4G hotspot, the returned value will
  // reflect WiFi, not 4G. This method may return SUBTYPE_UNKNOWN even if the
  // connection type is known.
  static content::mojom::ConnectionSubtype GetConnectionSubtype();

  // Returns true if |type| is a cellular connection.
  // Returns false if |type| is CONNECTION_UNKNOWN, and thus, depending on the
  // implementation of GetConnectionType(), it is possible that
  // IsConnectionCellular(GetConnectionType()) returns false even if the
  // current connection is cellular.
  static bool IsConnectionCellular(content::mojom::ConnectionType type);

  // Registers |observer| to receive notifications of network changes. The
  // thread on which this is called is the thread on which |observer| will be
  // called back with notifications.
  static void AddNetworkChangeObserver(NetworkChangeObserver* observer);

  // Unregisters |observer| from receiving notifications.  This must be called
  // on the same thread on which AddObserver() was called.
  static void RemoveNetworkChangeObserver(NetworkChangeObserver* observer);

 protected:
  // These are the actual implementations of the static queryable APIs.
  content::mojom::ConnectionType GetCurrentConnectionType() const;
  content::mojom::ConnectionSubtype GetCurrentConnectionSubtype() const;

 private:
  // NetworkChangeManagerClient implementation:
  void OnNetworkChanged(content::mojom::ConnectionType type,
                        content::mojom::ConnectionSubtype subtype) override;

  const scoped_refptr<base::ObserverListThreadSafe<NetworkChangeObserver>>
      network_change_observer_list_;

  DISALLOW_COPY_AND_ASSIGN(NetworkChangeManagerClientImpl);
};

}  // namespace network_change_manager_client

#endif  // CONTENT_NETWORK_NETWORK_CHANGE_MANAGER_CLIENT_IMPL_H_
