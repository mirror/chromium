// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_NETWORK_PROPERTIES_MANAGER_H_
#define COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_NETWORK_PROPERTIES_MANAGER_H_

#include <map>
#include <memory>
#include <string>
#include "base/macros.h"
#include "components/data_reduction_proxy/proto/network_properties.pb.h"

#include "base/macros.h"
#include "base/sequenced_task_runner.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_checker.h"
#include "base/values.h"
#include "components/prefs/pref_service.h"

namespace data_reduction_proxy {

// Stores the properties of a single network.
class NetworkPropertiesManager {
 public:
  NetworkPropertiesManager(
      PrefService* pref_service,
      scoped_refptr<base::SequencedTaskRunner> ui_task_runner,
      scoped_refptr<base::SingleThreadTaskRunner> io_task_runner);

  ~NetworkPropertiesManager();

  void OnChangeInNetworkID(const std::string& network_id);

  // Returns true if usage of secure proxies are allowed on the current network.
  bool IsSecureProxyAllowed() const;

  // Returns true if usage of insecure proxies are allowed on the current
  // network.
  bool IsInsecureProxyAllowed() const;

  // Returns true if usage of secure proxies has been disallowed by the carrier
  // on the current network.
  bool IsSecureProxyDisallowedByCarrier() const;

  // Sets the status of whether the usage of secure proxies is disallowed by the
  // carrier on the current network.
  void SetIsSecureProxyDisallowedByCarrier(bool disallowed_by_carrier);

  // Returns true if the current network has a captive portal.
  bool IsCaptivePortal() const;

  // Sets the status of whether the current network has a captive portal or not.
  // If the current network has captive portal, usage of secure proxies is
  // disallowed.
  void SetIsCaptivePortal(bool is_captive_portal);

  // If secure_proxy is true, returns true if the warmup URL probe has failed
  // on secure proxies on the current network. Otherwise, returns true if the
  // warmup URL probe has failed on insecure proxies.
  bool HasWarmupURLProbeFailed(bool secure_proxy) const;

  // Sets the status of whether the fetching of warmup URL failed on the current
  // network. Sets the status for secure proxies if |secure_proxy| is true.
  // Otherwise, sets the status for the insecure proxies.
  void SetHasWarmupURLProbeFailed(bool secure_proxy,
                                  bool warmup_url_probe_failed);

  // Map from network IDs to network properties.
  typedef std::map<std::string, NetworkProperties> ParsedPrefs;

  // Reads the prefs again, parses them, and returns the parsed map.
  ParsedPrefs ForceReadPrefsForTesting() const;

 private:
  void OnChangeInNetworkPropertyOnIOThread();

  void OnChangeInNetworkPropertyOnUIThread(
      const std::string& network_id,
      const NetworkProperties& network_properties);

  static ParsedPrefs ConvertDictionaryValueToParsedPrefs(
      const base::DictionaryValue* value);

  scoped_refptr<base::SequencedTaskRunner> ui_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;

  // Guaranteed to be non-null during the lifetime of |this|.
  PrefService* pref_service_;

  // |path_| is the location of the network quality estimator prefs.
  const std::string path_;

  // Current prefs on the disk. Should be accessed only on the UI thread.
  std::unique_ptr<base::DictionaryValue> prefs_;

  ParsedPrefs io_parsed_prefs_;

  // Thread safe: Can be accessed on any thread.
  const ParsedPrefs parsed_prefs_startup_;

  // ID of the current network. Must be accessed on the IO thread.
  std::string io_network_id_;

  // State of the proxies on the current network. Must be accessed on the IO
  // thread.
  NetworkProperties io_network_properties_;

  // Should be accessed only on the UI thread.
  base::WeakPtr<NetworkPropertiesManager> ui_weak_ptr_;

  // Used to get |weak_ptr_| to self on the UI thread.
  base::WeakPtrFactory<NetworkPropertiesManager> ui_weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(NetworkPropertiesManager);
};

}  // namespace data_reduction_proxy

#endif  // COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_NETWORK_PROPERTIES_MANAGER_H_