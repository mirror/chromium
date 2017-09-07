// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/networking_onc/networking_onc_event_router.h"

#include "base/json/json_writer.h"
#include "base/macros.h"
#include "chromeos/network/device_state.h"
#include "chromeos/network/network_certificate_handler.h"
#include "chromeos/network/network_event_log.h"
#include "chromeos/network/network_state.h"
#include "chromeos/network/network_state_handler.h"
#include "chromeos/network/network_state_handler_observer.h"
#include "chromeos/network/onc/onc_signature.h"
#include "chromeos/network/onc/onc_translator.h"
#include "chromeos/network/portal_detector/network_portal_detector.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/onc/onc_constants.h"
#include "content/public/browser/browser_context.h"
#include "extensions/browser/api/networking_onc/networking_onc_api.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/api/networking_onc.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

using chromeos::DeviceState;
using chromeos::NetworkHandler;
using chromeos::NetworkPortalDetector;
using chromeos::NetworkState;
using chromeos::NetworkStateHandler;

namespace extensions {

class NetworkingOncEventRouterImpl
    : public NetworkingOncEventRouter,
      public chromeos::NetworkStateHandlerObserver,
      public chromeos::NetworkCertificateHandler::Observer,
      public NetworkPortalDetector::Observer {
 public:
  explicit NetworkingOncEventRouterImpl(content::BrowserContext* context);
  ~NetworkingOncEventRouterImpl() override;

 protected:
  // KeyedService overrides:
  void Shutdown() override;

  // EventRouter::Observer overrides:
  void OnListenerAdded(const EventListenerInfo& details) override;
  void OnListenerRemoved(const EventListenerInfo& details) override;

  // NetworkStateHandlerObserver overrides:
  void NetworkListChanged() override;
  void DeviceListChanged() override;
  void NetworkPropertiesUpdated(const NetworkState* network) override;
  void DevicePropertiesUpdated(const DeviceState* device) override;

  // NetworkCertificateHandler::Observer overrides:
  void OnCertificatesChanged() override;

  // NetworkPortalDetector::Observer overrides:
  void OnPortalDetectionCompleted(
      const NetworkState* network,
      const NetworkPortalDetector::CaptivePortalState& state) override;

 private:
  // Decide if we should listen for network changes or not. If there are any
  // JavaScript listeners registered for the onNetworkChanged event, then we
  // want to register for change notification from the network state handler.
  // Otherwise, we want to unregister and not be listening to network changes.
  void StartOrStopListeningForNetworkChanges();

  content::BrowserContext* context_;
  bool listening_;

  DISALLOW_COPY_AND_ASSIGN(NetworkingOncEventRouterImpl);
};

NetworkingOncEventRouterImpl::NetworkingOncEventRouterImpl(
    content::BrowserContext* context)
    : context_(context), listening_(false) {
  // Register with the event router so we know when renderers are listening to
  // our events. We first check and see if there *is* an event router, because
  // some unit tests try to create all context services, but don't initialize
  // the event router first.
  EventRouter* event_router = EventRouter::Get(context_);
  if (event_router) {
    event_router->RegisterObserver(
        this, api::networking_onc::OnNetworksChanged::kEventName);
    event_router->RegisterObserver(
        this, api::networking_onc::OnNetworkListChanged::kEventName);
    event_router->RegisterObserver(
        this, api::networking_onc::OnDeviceStateListChanged::kEventName);
    event_router->RegisterObserver(
        this, api::networking_onc::OnPortalDetectionCompleted::kEventName);
    event_router->RegisterObserver(
        this, api::networking_onc::OnCertificateListsChanged::kEventName);
    StartOrStopListeningForNetworkChanges();
  }
}

NetworkingOncEventRouterImpl::~NetworkingOncEventRouterImpl() {
  DCHECK(!listening_);
}

void NetworkingOncEventRouterImpl::Shutdown() {
  // Unregister with the event router. We first check and see if there *is* an
  // event router, because some unit tests try to shutdown all context services,
  // but didn't initialize the event router first.
  EventRouter* event_router = EventRouter::Get(context_);
  if (event_router)
    event_router->UnregisterObserver(this);

  if (listening_) {
    NetworkHandler::Get()->network_state_handler()->RemoveObserver(this,
                                                                   FROM_HERE);
  }
  listening_ = false;
}

void NetworkingOncEventRouterImpl::OnListenerAdded(
    const EventListenerInfo& details) {
  // Start listening to events from the network state handler.
  StartOrStopListeningForNetworkChanges();
}

void NetworkingOncEventRouterImpl::OnListenerRemoved(
    const EventListenerInfo& details) {
  // Stop listening to events from the network state handler if there are no
  // more listeners.
  StartOrStopListeningForNetworkChanges();
}

void NetworkingOncEventRouterImpl::StartOrStopListeningForNetworkChanges() {
  EventRouter* event_router = EventRouter::Get(context_);
  bool should_listen =
      event_router->HasEventListener(
          api::networking_onc::OnNetworksChanged::kEventName) ||
      event_router->HasEventListener(
          api::networking_onc::OnNetworkListChanged::kEventName) ||
      event_router->HasEventListener(
          api::networking_onc::OnDeviceStateListChanged::kEventName) ||
      event_router->HasEventListener(
          api::networking_onc::OnPortalDetectionCompleted::kEventName) ||
      event_router->HasEventListener(
          api::networking_onc::OnCertificateListsChanged::kEventName);

  if (should_listen && !listening_) {
    NetworkHandler::Get()->network_state_handler()->AddObserver(this,
                                                                FROM_HERE);
    NetworkHandler::Get()->network_certificate_handler()->AddObserver(this);
    if (chromeos::network_portal_detector::IsInitialized())
      chromeos::network_portal_detector::GetInstance()->AddObserver(this);
  } else if (!should_listen && listening_) {
    NetworkHandler::Get()->network_state_handler()->RemoveObserver(this,
                                                                   FROM_HERE);
    NetworkHandler::Get()->network_certificate_handler()->RemoveObserver(this);
    if (chromeos::network_portal_detector::IsInitialized())
      chromeos::network_portal_detector::GetInstance()->RemoveObserver(this);
  }
  listening_ = should_listen;
}

void NetworkingOncEventRouterImpl::NetworkListChanged() {
  EventRouter* event_router = EventRouter::Get(context_);
  if (!event_router->HasEventListener(
          api::networking_onc::OnNetworkListChanged::kEventName)) {
    return;
  }

  NetworkStateHandler::NetworkStateList networks;
  NetworkHandler::Get()->network_state_handler()->GetVisibleNetworkList(
      &networks);
  std::vector<std::string> changes;
  for (NetworkStateHandler::NetworkStateList::const_iterator iter =
           networks.begin();
       iter != networks.end(); ++iter) {
    changes.push_back((*iter)->guid());
  }

  std::unique_ptr<base::ListValue> args(
      api::networking_onc::OnNetworkListChanged::Create(changes));
  std::unique_ptr<Event> extension_event(new Event(
      events::NETWORKING_PRIVATE_ON_NETWORK_LIST_CHANGED,
      api::networking_onc::OnNetworkListChanged::kEventName, std::move(args)));
  event_router->BroadcastEvent(std::move(extension_event));
}

void NetworkingOncEventRouterImpl::DeviceListChanged() {
  EventRouter* event_router = EventRouter::Get(context_);
  if (!event_router->HasEventListener(
          api::networking_onc::OnDeviceStateListChanged::kEventName)) {
    return;
  }

  std::unique_ptr<base::ListValue> args(
      api::networking_onc::OnDeviceStateListChanged::Create());
  std::unique_ptr<Event> extension_event(
      new Event(events::NETWORKING_PRIVATE_ON_DEVICE_STATE_LIST_CHANGED,
                api::networking_onc::OnDeviceStateListChanged::kEventName,
                std::move(args)));
  event_router->BroadcastEvent(std::move(extension_event));
}

void NetworkingOncEventRouterImpl::NetworkPropertiesUpdated(
    const NetworkState* network) {
  EventRouter* event_router = EventRouter::Get(context_);
  if (!event_router->HasEventListener(
          api::networking_onc::OnNetworksChanged::kEventName)) {
    NET_LOG_EVENT("NetworkingOnc.NetworkPropertiesUpdated: No Listeners",
                  network->path());
    return;
  }
  NET_LOG_EVENT("NetworkingOnc.NetworkPropertiesUpdated", network->path());
  std::unique_ptr<base::ListValue> args(
      api::networking_onc::OnNetworksChanged::Create(
          std::vector<std::string>(1, network->guid())));
  std::unique_ptr<Event> extension_event(new Event(
      events::NETWORKING_PRIVATE_ON_NETWORKS_CHANGED,
      api::networking_onc::OnNetworksChanged::kEventName, std::move(args)));
  event_router->BroadcastEvent(std::move(extension_event));
}

void NetworkingOncEventRouterImpl::DevicePropertiesUpdated(
    const DeviceState* device) {
  // DeviceState changes may affect Cellular networks.
  if (device->type() != shill::kTypeCellular)
    return;

  NetworkStateHandler::NetworkStateList cellular_networks;
  NetworkHandler::Get()->network_state_handler()->GetNetworkListByType(
      chromeos::NetworkTypePattern::Cellular(), false /* configured_only */,
      true /* visible_only */, -1 /* default limit */, &cellular_networks);
  for (const NetworkState* network : cellular_networks) {
    NetworkPropertiesUpdated(network);
  }
}

void NetworkingOncEventRouterImpl::OnCertificatesChanged() {
  EventRouter* event_router = EventRouter::Get(context_);
  if (!event_router->HasEventListener(
          api::networking_onc::OnCertificateListsChanged::kEventName)) {
    NET_LOG_EVENT("NetworkingOnc.OnCertificatesChanged: No Listeners", "");
    return;
  }
  NET_LOG_EVENT("NetworkingOnc.OnCertificatesChanged", "");

  std::unique_ptr<base::ListValue> args(
      api::networking_onc::OnCertificateListsChanged::Create());
  std::unique_ptr<Event> extension_event(
      new Event(events::NETWORKING_PRIVATE_ON_CERTIFICATE_LISTS_CHANGED,
                api::networking_onc::OnCertificateListsChanged::kEventName,
                std::move(args)));
  event_router->BroadcastEvent(std::move(extension_event));
}

void NetworkingOncEventRouterImpl::OnPortalDetectionCompleted(
    const NetworkState* network,
    const NetworkPortalDetector::CaptivePortalState& state) {
  const std::string path = network ? network->guid() : std::string();

  EventRouter* event_router = EventRouter::Get(context_);
  if (!event_router->HasEventListener(
          api::networking_onc::OnPortalDetectionCompleted::kEventName)) {
    NET_LOG_EVENT("NetworkingOnc.OnPortalDetectionCompleted: No Listeners",
                  path);
    return;
  }
  NET_LOG_EVENT("NetworkingOnc.OnPortalDetectionCompleted", path);

  api::networking_onc::CaptivePortalStatus status =
      api::networking_onc::CAPTIVE_PORTAL_STATUS_UNKNOWN;
  switch (state.status) {
    case NetworkPortalDetector::CAPTIVE_PORTAL_STATUS_UNKNOWN:
      status = api::networking_onc::CAPTIVE_PORTAL_STATUS_UNKNOWN;
      break;
    case NetworkPortalDetector::CAPTIVE_PORTAL_STATUS_OFFLINE:
      status = api::networking_onc::CAPTIVE_PORTAL_STATUS_OFFLINE;
      break;
    case NetworkPortalDetector::CAPTIVE_PORTAL_STATUS_ONLINE:
      status = api::networking_onc::CAPTIVE_PORTAL_STATUS_ONLINE;
      break;
    case NetworkPortalDetector::CAPTIVE_PORTAL_STATUS_PORTAL:
      status = api::networking_onc::CAPTIVE_PORTAL_STATUS_PORTAL;
      break;
    case NetworkPortalDetector::CAPTIVE_PORTAL_STATUS_PROXY_AUTH_REQUIRED:
      status = api::networking_onc::CAPTIVE_PORTAL_STATUS_PROXYAUTHREQUIRED;
      break;
    default:
      NOTREACHED();
      break;
  }

  std::unique_ptr<base::ListValue> args(
      api::networking_onc::OnPortalDetectionCompleted::Create(path, status));
  std::unique_ptr<Event> extension_event(
      new Event(events::NETWORKING_PRIVATE_ON_PORTAL_DETECTION_COMPLETED,
                api::networking_onc::OnPortalDetectionCompleted::kEventName,
                std::move(args)));
  event_router->BroadcastEvent(std::move(extension_event));
}

NetworkingOncEventRouter* NetworkingOncEventRouter::Create(
    content::BrowserContext* context) {
  return new NetworkingOncEventRouterImpl(context);
}

}  // namespace extensions
