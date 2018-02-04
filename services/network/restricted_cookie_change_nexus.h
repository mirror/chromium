// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_RESTRICTED_COOKIE_CHANGE_NEXUS_H_
#define SERVICES_NETWORK_RESTRICTED_COOKIE_CHANGE_NEXUS_H_

#include <memory>

#include "base/component_export.h"
#include "base/containers/linked_list.h"
#include "base/memory/weak_ptr.h"
#include "base/no_destructor.h"
#include "net/cookies/cookie_options.h"
#include "net/cookies/cookie_store.h"

namespace network {

// Dispatches cookie changes to multiple RestrictedCookieManager instances.
//
// This class is not thread-safe. Instances must be constructed, used, and
// destroyed on the network service's I/O thread.
//
// Adding cookie change listeners directly to CookieStore implementations is
// expensive. Specifically, adding N listeners requires O(N) CPU time to process
// any cookie change. Furthermore, listeners cannot be deleted. Thus, the
// CookieStore API is sufficient for connecting subsystems to CookieStores, but
// cannot be exposed (even indirectly) to Web pages. For this reason, a single
// RestrictedCookieChangeNexus instance is responsible for pooling all the
// listeners coming from RestrictedCookieManager (which is Web-exposed) into a
// single CookieStore change listener.
class COMPONENT_EXPORT(NETWORK_SERVICE) RestrictedCookieChangeNexus {
 public:
  class ListenerReference;

  // Exposed for testing.
  //
  // Production code should call RestrictedCookieChangeNexus::Instance()
  // instead of using this constructor directly. This class' exists to keep the
  // number of CookieMonster listeners down, and unnecessary instances go
  // against this mission.
  RestrictedCookieChangeNexus(net::CookieStore* cookie_store);

  // Implemented for testing.
  //
  // Production code uses a single RestrictedCookieChangeNexus instance that is
  // never destroyed.
  ~RestrictedCookieChangeNexus();

  // RestrictedCookieManager is a singleton, assuming a single CookieStore.
  //
  // This API makes it easy to adapt to the situation where a network service
  // might host multiple CookieStores.
  static RestrictedCookieChangeNexus* Instance(net::CookieStore* cookie_store);

  // Registers a callback that will receive cookie change notifications.
  //
  // The callback will be invoked synchronously on the network service's I/O
  // thread.
  //
  // The returned ListenerReference is a handler to the change subscription
  // created by this method. The callback is no longer invoked after the
  // ListenerReference is destroyed. Before a RestrictedCookieChangeNexus is
  // destroyed, all ListenerReference vended by it must be destroyed.
  ListenerReference AddListener(
      const GURL& url,
      const net::CookieOptions& options,
      net::CookieStore::CookieChangedCallback callback);

#if DCHECK_IS_ON()
  net::CookieStore* cookie_store() const { return cookie_store_; }
#endif  // DCHECK_IS_ON()

 private:
  // ListenerReference calls RemoveListener().
  friend class ListenerReference;
  class Listener;

  // Automatically called by ListenerReference's destructor.
  void RemoveListener(std::unique_ptr<Listener> listener);

  // Callback for net::CookieStore::AddCallbackForAllChanges().
  void OnCookieStoreChange(const net::CanonicalCookie& cookie,
                           net::CookieStore::ChangeCause change_cause);

  // The store whose changes are monitored.
  net::CookieStore* cookie_store_;

  // Subscription returned by CookieStore::AddCallbackForAllChanges().
  std::unique_ptr<net::CookieStore::CookieChangedSubscription>
      cookie_store_subscription_;

  // Active listeners.
  base::LinkedList<Listener> listeners_;

  // Vends the weak pointer used by the CookieStore listener.
  //
  // CookieMonster (the implementation of CookieStore) calls change listeners
  // asynchronously. RestrictedCookieChangeNexus removes its listener from
  // CoookieMonster on destruction, at which point CookieMonster may still have
  // change callbacks queued.
  base::WeakPtrFactory<RestrictedCookieChangeNexus> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(RestrictedCookieChangeNexus);
};

class RestrictedCookieChangeNexus::Listener : public base::LinkNode<Listener> {
 public:
  ~Listener();

  // Used by ListenerReference to remove the listener.
  RestrictedCookieChangeNexus* nexus() const { return nexus_; }

 private:
  // Calls the constructor and DispatchCookieChange().
  friend class RestrictedCookieChangeNexus;

  Listener(RestrictedCookieChangeNexus* nexus,
           const GURL& url,
           const net::CookieOptions& options,
           net::CookieStore::CookieChangedCallback callback_);

  void DispatchCookieChange(const net::CanonicalCookie& cookie,
                            net::CookieStore::ChangeCause change_cause);

  RestrictedCookieChangeNexus* nexus_;
  const GURL url_;
  const net::CookieOptions options_;
  const net::CookieStore::CookieChangedCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(Listener);
};

// Tied to the lifetime of a cookie change listener.
//
// When this instance goes out of scope, it removes the listener created by the
// RestrictedCookieChangeNexus::AddListener() call that returned the instance.
class RestrictedCookieChangeNexus::ListenerReference {
 public:
  // Remove the cookie change listener created by the method that returned this.
  ListenerReference() noexcept;
  ~ListenerReference();

  // References are passed by moving.
  ListenerReference(ListenerReference&&) noexcept;
  ListenerReference& operator=(ListenerReference&&) noexcept;

  Listener* listener() const { return listener_.get(); }

 private:
  // Calls the private constructor below.
  friend class RestrictedCookieChangeNexus;

  ListenerReference(std::unique_ptr<Listener> listener) noexcept;

  // Can be nullptr if the listener was moved out of this instance.
  std::unique_ptr<Listener> listener_;

  DISALLOW_COPY_AND_ASSIGN(ListenerReference);
};

}  // namespace network

#endif  // SERVICES_NETWORK_RESTRICTED_COOKIE_CHANGE_NEXUS_H_
