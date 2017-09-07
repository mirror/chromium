// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_NETWORKING_ONC_NETWORKING_ONC_SERVICE_CLIENT_H_
#define EXTENSIONS_BROWSER_API_NETWORKING_ONC_NETWORKING_ONC_SERVICE_CLIENT_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/containers/id_map.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/strings/string16.h"
#include "base/supports_user_data.h"
#include "base/threading/sequenced_worker_pool.h"
#include "base/values.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/wifi/wifi_service.h"
#include "content/public/browser/utility_process_host.h"
#include "content/public/browser/utility_process_host_client.h"
#include "extensions/browser/api/networking_onc/networking_onc_delegate.h"
#include "net/base/network_change_notifier.h"

namespace base {
class SequencedTaskRunner;
}

namespace extensions {

// Windows / Mac NetworkingOncApi implementation. This implements
// NetworkingOncDelegate, making WiFiService calls on the worker thead.
// It also observes |OnNetworkChanged| notifications and posts them to
// WiFiService on the worker thread. Created and called from the UI thread.
class NetworkingOncServiceClient
    : public NetworkingOncDelegate,
      net::NetworkChangeNotifier::NetworkChangeObserver {
 public:
  // Takes ownership of |wifi_service| which is accessed and deleted on the
  // worker thread. The deletion task is posted from the destructor.
  explicit NetworkingOncServiceClient(
      std::unique_ptr<wifi::WiFiService> wifi_service);

  // KeyedService
  void Shutdown() override;

  // NetworkingOncDelegate
  void GetProperties(const std::string& guid,
                     const DictionaryCallback& success_callback,
                     const FailureCallback& failure_callback) override;
  void GetManagedProperties(const std::string& guid,
                            const DictionaryCallback& success_callback,
                            const FailureCallback& failure_callback) override;
  void GetState(const std::string& guid,
                const DictionaryCallback& success_callback,
                const FailureCallback& failure_callback) override;
  void SetProperties(const std::string& guid,
                     std::unique_ptr<base::DictionaryValue> properties_dict,
                     bool allow_set_shared_config,
                     const VoidCallback& success_callback,
                     const FailureCallback& failure_callback) override;
  void CreateNetwork(bool shared,
                     std::unique_ptr<base::DictionaryValue> properties_dict,
                     const StringCallback& success_callback,
                     const FailureCallback& failure_callback) override;
  void ForgetNetwork(const std::string& guid,
                     bool allow_forget_shared_config,
                     const VoidCallback& success_callback,
                     const FailureCallback& failure_callback) override;
  void GetNetworks(const std::string& network_type,
                   bool configured_only,
                   bool visible_only,
                   int limit,
                   const NetworkListCallback& success_callback,
                   const FailureCallback& failure_callback) override;
  void StartConnect(const std::string& guid,
                    const VoidCallback& success_callback,
                    const FailureCallback& failure_callback) override;
  void StartDisconnect(const std::string& guid,
                       const VoidCallback& success_callback,
                       const FailureCallback& failure_callback) override;
  void SetWifiTDLSEnabledState(
      const std::string& ip_or_mac_address,
      bool enabled,
      const StringCallback& success_callback,
      const FailureCallback& failure_callback) override;
  void GetWifiTDLSStatus(const std::string& ip_or_mac_address,
                         const StringCallback& success_callback,
                         const FailureCallback& failure_callback) override;
  void GetCaptivePortalStatus(const std::string& guid,
                              const StringCallback& success_callback,
                              const FailureCallback& failure_callback) override;
  void UnlockCellularSim(const std::string& guid,
                         const std::string& pin,
                         const std::string& puk,
                         const VoidCallback& success_callback,
                         const FailureCallback& failure_callback) override;
  void SetCellularSimState(const std::string& guid,
                           bool require_pin,
                           const std::string& current_pin,
                           const std::string& new_pin,
                           const VoidCallback& success_callback,
                           const FailureCallback& failure_callback) override;
  std::unique_ptr<base::ListValue> GetEnabledNetworkTypes() override;
  std::unique_ptr<DeviceStateList> GetDeviceStateList() override;
  std::unique_ptr<base::DictionaryValue> GetGlobalPolicy() override;
  std::unique_ptr<base::DictionaryValue> GetCertificateLists() override;
  bool EnableNetworkType(const std::string& type) override;
  bool DisableNetworkType(const std::string& type) override;
  bool RequestScan(const std::string& type) override;
  void AddObserver(NetworkingOncDelegateObserver* observer) override;
  void RemoveObserver(NetworkingOncDelegateObserver* observer) override;

  // NetworkChangeNotifier::NetworkChangeObserver implementation.
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType type) override;

 private:
  // Callbacks to extension api function objects. Keep reference to API object
  // and are released in ShutdownOnUIThread. Run when WiFiService calls back
  // into NetworkingOncServiceClient's callback wrappers.
  typedef int32_t ServiceCallbacksID;
  struct ServiceCallbacks {
    ServiceCallbacks();
    ~ServiceCallbacks();

    DictionaryCallback get_properties_callback;
    VoidCallback start_connect_callback;
    VoidCallback start_disconnect_callback;
    VoidCallback set_properties_callback;
    StringCallback create_network_callback;
    NetworkListCallback get_visible_networks_callback;
    FailureCallback failure_callback;

    ServiceCallbacksID id;
  };
  using ServiceCallbacksMap = base::IDMap<std::unique_ptr<ServiceCallbacks>>;

  ~NetworkingOncServiceClient() override;

  // Callback wrappers.
  void AfterGetProperties(ServiceCallbacksID callback_id,
                          const std::string& network_guid,
                          std::unique_ptr<base::DictionaryValue> properties,
                          const std::string* error);
  void AfterSetProperties(ServiceCallbacksID callback_id,
                          const std::string* error);
  void AfterCreateNetwork(ServiceCallbacksID callback_id,
                          const std::string* network_guid,
                          const std::string* error);
  void AfterGetVisibleNetworks(ServiceCallbacksID callback_id,
                               std::unique_ptr<base::ListValue> networks);
  void AfterStartConnect(ServiceCallbacksID callback_id,
                         const std::string* error);
  void AfterStartDisconnect(ServiceCallbacksID callback_id,
                            const std::string* error);

  void OnNetworksChangedEventOnUIThread(
      const wifi::WiFiService::NetworkGuidList& network_guid_list);
  void OnNetworkListChangedEventOnUIThread(
      const wifi::WiFiService::NetworkGuidList& network_guid_list);

  // Add new |ServiceCallbacks| to |callbacks_map_|.
  ServiceCallbacks* AddServiceCallbacks();
  // Removes ServiceCallbacks for |callback_id| from |callbacks_map_|.
  void RemoveServiceCallbacks(ServiceCallbacksID callback_id);

  // Callbacks to run when callback is called from WiFiService.
  ServiceCallbacksMap callbacks_map_;
  // Observers to Network Events.
  base::ObserverList<NetworkingOncDelegateObserver> network_events_observers_;
  // Interface to WiFiService. Used and deleted on the worker thread.
  std::unique_ptr<wifi::WiFiService> wifi_service_;
  // Task runner for worker tasks.
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  // Use WeakPtrs for callbacks from |wifi_service_|.
  base::WeakPtrFactory<NetworkingOncServiceClient> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(NetworkingOncServiceClient);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_NETWORKING_ONC_NETWORKING_ONC_SERVICE_CLIENT_H_
