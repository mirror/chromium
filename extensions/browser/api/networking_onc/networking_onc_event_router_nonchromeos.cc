// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/networking_onc/networking_onc_event_router.h"

#include "base/macros.h"
#include "content/public/browser/browser_context.h"
#include "extensions/browser/api/networking_onc/networking_onc_api.h"
#include "extensions/browser/api/networking_onc/networking_onc_delegate_factory.h"
#include "extensions/browser/api/networking_onc/networking_onc_delegate_observer.h"
#include "extensions/common/api/networking_onc.h"

namespace extensions {

// This is an event router that will observe listeners to |NetworksChanged| and
// |NetworkListChanged| events.
class NetworkingOncEventRouterImpl : public NetworkingOncEventRouter,
                                     public NetworkingOncDelegateObserver {
 public:
  explicit NetworkingOncEventRouterImpl(
      content::BrowserContext* browser_context);
  ~NetworkingOncEventRouterImpl() override;

 protected:
  // KeyedService overrides:
  void Shutdown() override;

  // EventRouter::Observer overrides:
  void OnListenerAdded(const EventListenerInfo& details) override;
  void OnListenerRemoved(const EventListenerInfo& details) override;

  // NetworkingOncDelegateObserver overrides:
  void OnNetworksChangedEvent(
      const std::vector<std::string>& network_guids) override;
  void OnNetworkListChangedEvent(
      const std::vector<std::string>& network_guids) override;

 private:
  // Decide if we should listen for network changes or not. If there are any
  // JavaScript listeners registered for the onNetworkChanged event, then we
  // want to register for change notification from the network state handler.
  // Otherwise, we want to unregister and not be listening to network changes.
  void StartOrStopListeningForNetworkChanges();

  content::BrowserContext* browser_context_;
  bool listening_;

  DISALLOW_COPY_AND_ASSIGN(NetworkingOncEventRouterImpl);
};

NetworkingOncEventRouterImpl::NetworkingOncEventRouterImpl(
    content::BrowserContext* browser_context)
    : browser_context_(browser_context), listening_(false) {
  // Register with the event router so we know when renderers are listening to
  // our events. We first check and see if there *is* an event router, because
  // some unit tests try to create all profile services, but don't initialize
  // the event router first.
  EventRouter* event_router = EventRouter::Get(browser_context_);
  if (!event_router)
    return;
  event_router->RegisterObserver(
      this, api::networking_onc::OnNetworksChanged::kEventName);
  event_router->RegisterObserver(
      this, api::networking_onc::OnNetworkListChanged::kEventName);
  StartOrStopListeningForNetworkChanges();
}

NetworkingOncEventRouterImpl::~NetworkingOncEventRouterImpl() {
  DCHECK(!listening_);
}

void NetworkingOncEventRouterImpl::Shutdown() {
  // Unregister with the event router. We first check and see if there *is* an
  // event router, because some unit tests try to shutdown all profile services,
  // but didn't initialize the event router first.
  EventRouter* event_router = EventRouter::Get(browser_context_);
  if (event_router)
    event_router->UnregisterObserver(this);

  if (!listening_)
    return;
  listening_ = false;
  NetworkingOncDelegate* delegate =
      NetworkingOncDelegateFactory::GetForBrowserContext(browser_context_);
  if (delegate)
    delegate->RemoveObserver(this);
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
  EventRouter* event_router = EventRouter::Get(browser_context_);
  if (!event_router)
    return;

  bool should_listen =
      event_router->HasEventListener(
          api::networking_onc::OnNetworksChanged::kEventName) ||
      event_router->HasEventListener(
          api::networking_onc::OnNetworkListChanged::kEventName);

  if (should_listen && !listening_) {
    NetworkingOncDelegate* delegate =
        NetworkingOncDelegateFactory::GetForBrowserContext(browser_context_);
    if (delegate)
      delegate->AddObserver(this);
  }
  if (!should_listen && listening_) {
    NetworkingOncDelegate* delegate =
        NetworkingOncDelegateFactory::GetForBrowserContext(browser_context_);
    if (delegate)
      delegate->RemoveObserver(this);
  }

  listening_ = should_listen;
}

void NetworkingOncEventRouterImpl::OnNetworksChangedEvent(
    const std::vector<std::string>& network_guids) {
  EventRouter* event_router = EventRouter::Get(browser_context_);
  if (!event_router)
    return;
  std::unique_ptr<base::ListValue> args(
      api::networking_onc::OnNetworksChanged::Create(network_guids));
  std::unique_ptr<Event> netchanged_event(new Event(
      events::NETWORKING_PRIVATE_ON_NETWORKS_CHANGED,
      api::networking_onc::OnNetworksChanged::kEventName, std::move(args)));
  event_router->BroadcastEvent(std::move(netchanged_event));
}

void NetworkingOncEventRouterImpl::OnNetworkListChangedEvent(
    const std::vector<std::string>& network_guids) {
  EventRouter* event_router = EventRouter::Get(browser_context_);
  if (!event_router)
    return;
  std::unique_ptr<base::ListValue> args(
      api::networking_onc::OnNetworkListChanged::Create(network_guids));
  std::unique_ptr<Event> netlistchanged_event(new Event(
      events::NETWORKING_PRIVATE_ON_NETWORK_LIST_CHANGED,
      api::networking_onc::OnNetworkListChanged::kEventName, std::move(args)));
  event_router->BroadcastEvent(std::move(netlistchanged_event));
}

NetworkingOncEventRouter* NetworkingOncEventRouter::Create(
    content::BrowserContext* browser_context) {
  return new NetworkingOncEventRouterImpl(browser_context);
}

}  // namespace extensions
