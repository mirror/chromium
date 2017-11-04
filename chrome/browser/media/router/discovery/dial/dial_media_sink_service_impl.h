// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_DIAL_DIAL_MEDIA_SINK_SERVICE_IMPL_H_
#define CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_DIAL_DIAL_MEDIA_SINK_SERVICE_IMPL_H_

#include <memory>
#include <set>

#include "base/memory/weak_ptr.h"
#include "chrome/browser/media/router/discovery/dial/device_description_service.h"
#include "chrome/browser/media/router/discovery/dial/dial_app_discovery_service.h"
#include "chrome/browser/media/router/discovery/dial/dial_registry.h"
#include "chrome/browser/media/router/discovery/media_sink_discovery_metrics.h"
#include "chrome/browser/media/router/discovery/media_sink_service_base.h"

namespace media_router {

class DeviceDescriptionService;
class DialRegistry;

// An observer class registered with DialMediaSinkService to receive
// notifications when DIAL sinks are added or removed from DialMediaSinkService
class DialMediaSinkServiceObserver {
 public:
  virtual ~DialMediaSinkServiceObserver() {}

  // Invoked when |sink| is added to DialMediaSinkServiceImpl instance.
  // |sink|: must be a DIAL sink.
  virtual void OnDialSinkAdded(const MediaSinkInternal& sink) = 0;
};

// A service which can be used to start background discovery and resolution of
// DIAL devices (Smart TVs, Game Consoles, etc.).
// This class is not thread safe. All methods must be called from the IO thread.
class DialMediaSinkServiceImpl
    : public MediaSinkServiceBase,
      public DialRegistry::Observer,
      public base::SupportsWeakPtr<DialMediaSinkServiceImpl> {
 public:
  // Called when a new sink becomes available or an available sink becomes
  // unavailable for |app_name|.
  // |app_name|: app name on receiver device (e.g. YouTube)
  // |available_sinks|: list of currently available sinks for |app_name|
  using OnAvailableSinksUpdatedCallback =
      base::Callback<void(const std::string& app_name,
                          const std::vector<MediaSink>& available_sinks)>;

  DialMediaSinkServiceImpl(
      const OnSinksDiscoveredCallback& callback,
      const OnAvailableSinksUpdatedCallback& available_sinks_updated_callback,
      net::URLRequestContextGetter* request_context);
  ~DialMediaSinkServiceImpl() override;

  // Does not take ownership of |observer|. Caller should make sure |observer|
  // object outlives |this|.
  void SetObserver(DialMediaSinkServiceObserver* observer);

  // Sets |observer_| to nullptr.
  void ClearObserver();

  // Starts monitoring available sinks for |app_name|. If available sinks
  // change, invokes |available_sinks_updated_callback_|.
  void StartMonitoringAvailableSinksForApp(const std::string& app_name);

  // Stops monitoring available sinks for |app_name|.
  void StopMonitoringAvailableSinksForApp(const std::string& app_name);

  // MediaSinkService implementation
  void Start() override;
  void Stop() override;
  void OnUserGesture() override;

 protected:
  // Returns instance of device description service. Create a new one if none
  // exists.
  DeviceDescriptionService* GetDescriptionService();

  // Does not take ownership of |dial_registry|.
  void SetDialRegistryForTest(DialRegistry* dial_registry);
  void SetDescriptionServiceForTest(
      std::unique_ptr<DeviceDescriptionService> description_service);

 private:
  friend class DialMediaSinkServiceImplTest;
  FRIEND_TEST_ALL_PREFIXES(DialMediaSinkServiceImplTest, TestStart);
  FRIEND_TEST_ALL_PREFIXES(DialMediaSinkServiceImplTest, TestTimer);
  FRIEND_TEST_ALL_PREFIXES(DialMediaSinkServiceImplTest,
                           TestOnDeviceDescriptionAvailable);
  FRIEND_TEST_ALL_PREFIXES(DialMediaSinkServiceImplTest, TestRestartAfterStop);
  FRIEND_TEST_ALL_PREFIXES(DialMediaSinkServiceImplTest,
                           OnDialSinkAddedCalledOnUserGesture);
  FRIEND_TEST_ALL_PREFIXES(DialMediaSinkServiceImplTest,
                           DialMediaSinkServiceObserver);
  // DialRegistry::Observer implementation
  void OnDialDeviceEvent(const DialRegistry::DeviceList& devices) override;
  void OnDialError(DialRegistry::DialErrorCode type) override;

  // Called when description service successfully fetches and parses device
  // description XML. Restart |finish_timer_| if it is not running.
  void OnDeviceDescriptionAvailable(
      const DialDeviceData& device_data,
      const ParsedDialDeviceDescription& description_data);

  // Called when fails to fetch or parse device description XML.
  void OnDeviceDescriptionError(const DialDeviceData& device,
                                const std::string& error_message);

  // Called when app discovery service successfully fetches and parses app info
  // XML.
  void OnDialAppInfoAvailable(const GURL& app_url,
                              const ParsedDialAppInfo& app_info);

  // Called when fails to fetch or parse app info XML.
  void OnDialAppInfoError(const GURL& app_url,
                          const std::string& error_message);

  void OnDialAppInfoUpdated(const GURL& app_url, bool is_available);

  void NotifySinkObservers(const std::string& app_name,
                           const std::set<std::string>& available_sink_id_set);

  // MediaSinkServiceBase implementation.
  void RecordDeviceCounts() override;

  std::unique_ptr<DeviceDescriptionService> description_service_;

  std::unique_ptr<DialAppDiscoveryService> app_discovery_service_;

  OnAvailableSinksUpdatedCallback available_sinks_updated_callback_;

  // Raw pointer to DialRegistry singleton.
  DialRegistry* dial_registry_ = nullptr;

  // DialRegistry for unit test.
  DialRegistry* test_dial_registry_ = nullptr;

  // Device data list from current round of discovery.
  DialRegistry::DeviceList current_devices_;

  // Map of sets of available sink ids keyed by app name.
  std::map<std::string, std::set<std::string>> app_name_sink_id_map_;

  DialMediaSinkServiceObserver* observer_;

  scoped_refptr<net::URLRequestContextGetter> request_context_;

  DialDeviceCountMetrics metrics_;
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_DIAL_DIAL_MEDIA_SINK_SERVICE_IMPL_H_
