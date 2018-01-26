// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_NETWORK_RESTRICTED_COOKIE_CHANGE_NEXUS_H_
#define CONTENT_NETWORK_RESTRICTED_COOKIE_CHANGE_NEXUS_H_

#include "base/containers/linked_list.h"
#include "base/no_destructor.h"
#include "net/cookies/cookie_options.h"
#include "net/cookies/cookie_store.h"

namespace content {

// Dispatches cookie changes to multiple RestrictedCookieManager instances.
//
// Adding cookie change listeners directly to CookieStore implementations is
// expensive. Specifically, adding N listeners requires O(N) CPU time to process
// any cookie change. Furthermore, listeners cannot be deleted. Thus, the
// CookieStore API is sufficient for connecting subsystems to CookieStores, but
// cannot be exposed (even indirectly) to Web pages. For this reason, a single
// RestrictedCookieChangeNexus instance is responsible for pooling all the
// listeners coming from RestrictedCookieManager (which is Web-exposed) into a
// single CookieStore change listener.
class RestrictedCookieChangeNexus {
 public:
  class ListenerReference;

  // No copying.
  RestrictedCookieChangeNexus(const RestrictedCookieChangeNexus&) = delete;
  RestrictedCookieChangeNexus& operator=(const RestrictedCookieChangeNexus&) =
      delete;

  // RestrictedCookieManager is a singleton, assuming a single CookieStore.
  //
  // This API makes it easy to adapt to the situation where a network service
  // might host multiple CookieStores.
  static RestrictedCookieChangeNexus* Instance(net::CookieStore* cookie_store);

  //
  ListenerReference AddListener(
      const GURL& url,
      const net::CookieOptions& options,
      net::CookieStore::CookieChangedCallback callback);

 private:
  friend class base::NoDestructor<RestrictedCookieChangeNexus>;

  // ListenerReference calls RemoveListener().
  friend class ListenerReference;
  class Listener;

  RestrictedCookieChangeNexus(net::CookieStore* cookie_store);

  // Static instances are never deleted.
  ~RestrictedCookieChangeNexus() = delete;

  //
  void RemoveListener(base::LinkNode<Listener>* listener_node);

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
};

class RestrictedCookieChangeNexus::Listener : public base::LinkNode<Listener> {
 public:
  ~Listener();

  // Prevent accidental copying.
  Listener(const Listener&) = delete;
  Listener& operator=(const Listener&) = delete;

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

  // No copying.
  ListenerReference(const ListenerReference&) = delete;
  ListenerReference& operator=(const ListenerReference&) = delete;

  // References are passed by moving.
  ListenerReference(ListenerReference&&) noexcept;
  ListenerReference& operator=(ListenerReference&&) noexcept;

 private:
  // Calls the private constructor below.
  friend class RestrictedCookieChangeNexus;

  ListenerReference(std::unique_ptr<Listener> listener_node) noexcept;

  // Can be nullptr if the listener was moved out of this instance.
  std::unique_ptr<Listener> listener_node_;
};

}  // namespace content

#endif  // CONTENT_NETWORK_RESTRICTED_COOKIE_CHANGE_NEXUS_H_