// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_RESTRICTED_COOKIE_MANAGER_H_
#define SERVICES_NETWORK_RESTRICTED_COOKIE_MANAGER_H_

#include <string>

#include "base/component_export.h"
#include "base/containers/linked_list.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/cookies/canonical_cookie.h"
#include "net/cookies/cookie_store.h"
#include "services/network/public/interfaces/restricted_cookie_manager.mojom.h"
#include "services/network/restricted_cookie_change_nexus.h"
#include "url/gurl.h"

namespace net {
class CookieStore;
}  // namespace net

namespace network {

// RestrictedCookieManager implementation.
//
// Instances of this class must be created and used on the I/O thread. Instances
// are created by CreateMojoService() and are bound to the lifetimes of the
// mojo connections that they serve, via mojo::StrongBinding.
class COMPONENT_EXPORT(NETWORK_SERVICE) RestrictedCookieManager
    : public mojom::RestrictedCookieManager {
 public:
  // |change_nexus| is exposed for dependency injection in tests. It is
  // always RestrictedCookieChangeNexus::Instance(cookie_store) in production.
  RestrictedCookieManager(net::CookieStore* cookie_store,
                          RestrictedCookieChangeNexus* cookie_change_nexus,
                          int render_process_id,
                          int render_frame_id);
  ~RestrictedCookieManager() override;

  // Implements CookieStore.getAll() in the Async Cookies API.
  //
  // This method is also used as the backend for CookieStore.get() and
  // CookieStore.has().
  void GetAllForUrl(const GURL& url,
                    const GURL& site_for_cookies,
                    mojom::CookieManagerGetOptionsPtr options,
                    GetAllForUrlCallback callback) override;

  void SetCanonicalCookie(const net::CanonicalCookie& cookie,
                          const GURL& url,
                          const GURL& site_for_cookies,
                          SetCanonicalCookieCallback callback) override;

  void AddChangeListener(
      const GURL& url,
      const GURL& site_for_cookies,
      network::mojom::RestrictedCookieChangeListenerPtr listener) override;

 private:
  // base::LinkedList container for a RestrictedCookieChangeNexus listener.
  class NexusListenerReferenceNode;

  // Feeds a net::CookieList to a GetAllForUrl() callback.
  void CookieListToGetAllForUrlCallback(
      const GURL& url,
      const GURL& site_for_cookies,
      mojom::CookieManagerGetOptionsPtr options,
      GetAllForUrlCallback callback,
      const net::CookieList& cookie_list);

  // Called when the Mojo pipe associated with a listener is closed.
  void RemoveChangeListener(
      base::LinkNode<NexusListenerReferenceNode>* listener_node);

  // Callback for net::CookieStore::AddCallbackForAllChanges().
  void OnCookieChange(
      const network::mojom::RestrictedCookieChangeListenerPtr& listener,
      const net::CanonicalCookie& cookie,
      net::CookieStore::ChangeCause change_cause);

  net::CookieStore* const cookie_store_;
  const int render_process_id_;
  const int render_frame_id_;

  RestrictedCookieChangeNexus* const cookie_change_nexus_;
  base::LinkedList<NexusListenerReferenceNode> listeners_;

  base::WeakPtrFactory<RestrictedCookieManager> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(RestrictedCookieManager);
};

}  // namespace network

#endif  // SERVICES_NETWORK_RESTRICTED_COOKIE_MANAGER_H_
