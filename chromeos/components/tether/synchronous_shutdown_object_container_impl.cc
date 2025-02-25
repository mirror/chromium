// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/synchronous_shutdown_object_container_impl.h"

#include <memory>

#include "base/time/default_clock.h"
#include "chromeos/components/tether/active_host.h"
#include "chromeos/components/tether/active_host_network_state_updater.h"
#include "chromeos/components/tether/asynchronous_shutdown_object_container.h"
#include "chromeos/components/tether/device_id_tether_network_guid_map.h"
#include "chromeos/components/tether/host_connection_metrics_logger.h"
#include "chromeos/components/tether/host_scan_device_prioritizer_impl.h"
#include "chromeos/components/tether/host_scan_scheduler_impl.h"
#include "chromeos/components/tether/host_scanner.h"
#include "chromeos/components/tether/hotspot_usage_duration_tracker.h"
#include "chromeos/components/tether/keep_alive_scheduler.h"
#include "chromeos/components/tether/master_host_scan_cache.h"
#include "chromeos/components/tether/network_connection_handler_tether_delegate.h"
#include "chromeos/components/tether/network_host_scan_cache.h"
#include "chromeos/components/tether/network_list_sorter.h"
#include "chromeos/components/tether/notification_remover.h"
#include "chromeos/components/tether/persistent_host_scan_cache_impl.h"
#include "chromeos/components/tether/tether_connector_impl.h"
#include "chromeos/components/tether/tether_disconnector_impl.h"
#include "chromeos/components/tether/tether_host_response_recorder.h"
#include "chromeos/components/tether/tether_network_disconnection_handler.h"
#include "chromeos/components/tether/tether_session_completion_logger.h"
#include "chromeos/components/tether/timer_factory.h"
#include "chromeos/components/tether/wifi_hotspot_connector.h"

namespace chromeos {

namespace tether {

// static
SynchronousShutdownObjectContainerImpl::Factory*
    SynchronousShutdownObjectContainerImpl::Factory::factory_instance_ =
        nullptr;

// static
std::unique_ptr<SynchronousShutdownObjectContainer>
SynchronousShutdownObjectContainerImpl::Factory::NewInstance(
    AsynchronousShutdownObjectContainer* asychronous_container,
    NotificationPresenter* notification_presenter,
    GmsCoreNotificationsStateTrackerImpl* gms_core_notifications_state_tracker,
    PrefService* pref_service,
    NetworkStateHandler* network_state_handler,
    NetworkConnect* network_connect,
    NetworkConnectionHandler* network_connection_handler) {
  if (!factory_instance_)
    factory_instance_ = new Factory();

  return factory_instance_->BuildInstance(
      asychronous_container, notification_presenter,
      gms_core_notifications_state_tracker, pref_service, network_state_handler,
      network_connect, network_connection_handler);
}

// static
void SynchronousShutdownObjectContainerImpl::Factory::SetInstanceForTesting(
    Factory* factory) {
  factory_instance_ = factory;
}

SynchronousShutdownObjectContainerImpl::Factory::~Factory() = default;

std::unique_ptr<SynchronousShutdownObjectContainer>
SynchronousShutdownObjectContainerImpl::Factory::BuildInstance(
    AsynchronousShutdownObjectContainer* asychronous_container,
    NotificationPresenter* notification_presenter,
    GmsCoreNotificationsStateTrackerImpl* gms_core_notifications_state_tracker,
    PrefService* pref_service,
    NetworkStateHandler* network_state_handler,
    NetworkConnect* network_connect,
    NetworkConnectionHandler* network_connection_handler) {
  return base::WrapUnique(new SynchronousShutdownObjectContainerImpl(
      asychronous_container, notification_presenter,
      gms_core_notifications_state_tracker, pref_service, network_state_handler,
      network_connect, network_connection_handler));
}

SynchronousShutdownObjectContainerImpl::SynchronousShutdownObjectContainerImpl(
    AsynchronousShutdownObjectContainer* asychronous_container,
    NotificationPresenter* notification_presenter,
    GmsCoreNotificationsStateTrackerImpl* gms_core_notifications_state_tracker,
    PrefService* pref_service,
    NetworkStateHandler* network_state_handler,
    NetworkConnect* network_connect,
    NetworkConnectionHandler* network_connection_handler)
    : network_state_handler_(network_state_handler),
      network_list_sorter_(std::make_unique<NetworkListSorter>()),
      tether_host_response_recorder_(
          std::make_unique<TetherHostResponseRecorder>(pref_service)),
      device_id_tether_network_guid_map_(
          std::make_unique<DeviceIdTetherNetworkGuidMap>()),
      tether_session_completion_logger_(
          std::make_unique<TetherSessionCompletionLogger>()),
      host_scan_device_prioritizer_(
          std::make_unique<HostScanDevicePrioritizerImpl>(
              tether_host_response_recorder_.get())),
      wifi_hotspot_connector_(
          std::make_unique<WifiHotspotConnector>(network_state_handler_,
                                                 network_connect)),
      active_host_(std::make_unique<ActiveHost>(
          asychronous_container->tether_host_fetcher(),
          pref_service)),
      active_host_network_state_updater_(
          std::make_unique<ActiveHostNetworkStateUpdater>(
              active_host_.get(),
              network_state_handler_)),
      persistent_host_scan_cache_(
          std::make_unique<PersistentHostScanCacheImpl>(pref_service)),
      network_host_scan_cache_(std::make_unique<NetworkHostScanCache>(
          network_state_handler_,
          tether_host_response_recorder_.get(),
          device_id_tether_network_guid_map_.get())),
      master_host_scan_cache_(std::make_unique<MasterHostScanCache>(
          std::make_unique<TimerFactory>(),
          active_host_.get(),
          network_host_scan_cache_.get(),
          persistent_host_scan_cache_.get())),
      notification_remover_(
          std::make_unique<NotificationRemover>(network_state_handler_,
                                                notification_presenter,
                                                master_host_scan_cache_.get(),
                                                active_host_.get())),
      keep_alive_scheduler_(std::make_unique<KeepAliveScheduler>(
          active_host_.get(),
          asychronous_container->ble_connection_manager(),
          master_host_scan_cache_.get(),
          device_id_tether_network_guid_map_.get())),
      clock_(std::make_unique<base::DefaultClock>()),
      hotspot_usage_duration_tracker_(
          std::make_unique<HotspotUsageDurationTracker>(active_host_.get(),
                                                        clock_.get())),
      host_scanner_(std::make_unique<HostScanner>(
          network_state_handler_,
          asychronous_container->tether_host_fetcher(),
          asychronous_container->ble_connection_manager(),
          host_scan_device_prioritizer_.get(),
          tether_host_response_recorder_.get(),
          gms_core_notifications_state_tracker,
          notification_presenter,
          device_id_tether_network_guid_map_.get(),
          master_host_scan_cache_.get(),
          clock_.get())),
      host_scan_scheduler_(
          std::make_unique<HostScanSchedulerImpl>(network_state_handler_,
                                                  host_scanner_.get())),
      host_connection_metrics_logger_(
          std::make_unique<HostConnectionMetricsLogger>()),
      tether_connector_(std::make_unique<TetherConnectorImpl>(
          network_state_handler_,
          wifi_hotspot_connector_.get(),
          active_host_.get(),
          asychronous_container->tether_host_fetcher(),
          asychronous_container->ble_connection_manager(),
          tether_host_response_recorder_.get(),
          device_id_tether_network_guid_map_.get(),
          master_host_scan_cache_.get(),
          notification_presenter,
          host_connection_metrics_logger_.get(),
          asychronous_container->disconnect_tethering_request_sender(),
          asychronous_container->wifi_hotspot_disconnector(),
          clock_.get())),
      tether_disconnector_(std::make_unique<TetherDisconnectorImpl>(
          active_host_.get(),
          asychronous_container->wifi_hotspot_disconnector(),
          asychronous_container->disconnect_tethering_request_sender(),
          tether_connector_.get(),
          device_id_tether_network_guid_map_.get(),
          tether_session_completion_logger_.get())),
      tether_network_disconnection_handler_(
          std::make_unique<TetherNetworkDisconnectionHandler>(
              active_host_.get(),
              network_state_handler_,
              asychronous_container->network_configuration_remover(),
              asychronous_container->disconnect_tethering_request_sender(),
              tether_session_completion_logger_.get())),
      network_connection_handler_tether_delegate_(
          std::make_unique<NetworkConnectionHandlerTetherDelegate>(
              network_connection_handler,
              active_host_.get(),
              tether_connector_.get(),
              tether_disconnector_.get())) {
  network_state_handler_->set_tether_sort_delegate(network_list_sorter_.get());
}

SynchronousShutdownObjectContainerImpl::
    ~SynchronousShutdownObjectContainerImpl() {
  network_state_handler_->set_tether_sort_delegate(nullptr);
}

ActiveHost* SynchronousShutdownObjectContainerImpl::active_host() {
  return active_host_.get();
}

HostScanCache* SynchronousShutdownObjectContainerImpl::host_scan_cache() {
  return master_host_scan_cache_.get();
}

HostScanScheduler*
SynchronousShutdownObjectContainerImpl::host_scan_scheduler() {
  return host_scan_scheduler_.get();
}

TetherDisconnector*
SynchronousShutdownObjectContainerImpl::tether_disconnector() {
  return tether_disconnector_.get();
}

}  // namespace tether

}  // namespace chromeos
