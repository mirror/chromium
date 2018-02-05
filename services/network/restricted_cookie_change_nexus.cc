// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/restricted_cookie_change_nexus.h"

#include <memory>

#include "base/bind.h"
#include "base/no_destructor.h"
#include "net/cookies/canonical_cookie.h"
#include "net/cookies/cookie_options.h"
#include "net/cookies/cookie_store.h"

namespace network {

RestrictedCookieChangeNexus::Listener::~Listener() = default;

RestrictedCookieChangeNexus::Listener::Listener(
    RestrictedCookieChangeNexus* nexus,
    const GURL& url,
    const net::CookieOptions& options,
    net::CookieStore::CookieChangedCallback callback)
    : nexus_(nexus),
      url_(url),
      options_(options),
      callback_(std::move(callback)) {}

void RestrictedCookieChangeNexus::Listener::DispatchCookieChange(
    const net::CanonicalCookie& cookie,
    net::CookieStore::ChangeCause change_cause) {
  if (!cookie.IncludeForRequestURL(url_, options_))
    return;

  callback_.Run(cookie, change_cause);
}

RestrictedCookieChangeNexus::ListenerReference::ListenerReference() noexcept =
    default;
RestrictedCookieChangeNexus::ListenerReference::~ListenerReference() {
  if (!listener_)
    return;

  listener_->nexus()->RemoveListener(std::move(listener_));
}

RestrictedCookieChangeNexus::ListenerReference::ListenerReference(
    ListenerReference&&) noexcept = default;

RestrictedCookieChangeNexus::ListenerReference&
RestrictedCookieChangeNexus::ListenerReference::operator=(
    ListenerReference&&) noexcept = default;

RestrictedCookieChangeNexus::ListenerReference::ListenerReference(
    std::unique_ptr<Listener> listener) noexcept
    : listener_(std::move(listener)) {}

RestrictedCookieChangeNexus::RestrictedCookieChangeNexus(
    net::CookieStore* cookie_store)
    : cookie_store_(cookie_store), weak_ptr_factory_(this) {}

RestrictedCookieChangeNexus::~RestrictedCookieChangeNexus() {
  DCHECK(listeners_.empty())
      << "All listeners must be removed before destruction deleted";

  // The destructor of cookie_store_subscription_ will remove the CookieStore
  // change listener.
}

// static
RestrictedCookieChangeNexus* RestrictedCookieChangeNexus::Instance(
    net::CookieStore* cookie_store) {
  static base::NoDestructor<RestrictedCookieChangeNexus> singleton_instance(
      cookie_store);

  // If this DCHECK fails, we'll need to replace the singleton instance with a
  // base::NoDestructor<base::flat_map<net::CookieStore*, CookieChangeNexus*>>.
  DCHECK_EQ(cookie_store, singleton_instance->cookie_store_)
      << "CookieChangeNexus singleton needs to handle multiple CookieStores";
  return singleton_instance.get();
}

RestrictedCookieChangeNexus::ListenerReference
RestrictedCookieChangeNexus::AddListener(
    const GURL& url,
    const net::CookieOptions& options,
    net::CookieStore::CookieChangedCallback callback) {
  // TODO(pwnall): If the lifecycles of RestrictedCookieChangeNexus and
  //     CookieMonster can be coordinated, AddCallbackForAllChanges can be
  //     called once in the constructor, and cookie_store_subscription_ would
  //     never be reset.
  if (!cookie_store_subscription_) {
    DCHECK(listeners_.empty());
    cookie_store_subscription_ = cookie_store_->AddCallbackForAllChanges(
        base::BindRepeating(&RestrictedCookieChangeNexus::OnCookieStoreChange,
                            weak_ptr_factory_.GetWeakPtr()));
  }

  ListenerReference listener_reference =
      ListenerReference(std::unique_ptr<Listener>(
          new Listener(this, url, options, std::move(callback))));
  listeners_.Append(listener_reference.listener());
  return listener_reference;
}

void RestrictedCookieChangeNexus::RemoveListener(
    std::unique_ptr<Listener> listener) {
  listener->RemoveFromList();
  if (listeners_.empty())
    cookie_store_subscription_.reset();
}

void RestrictedCookieChangeNexus::OnCookieStoreChange(
    const net::CanonicalCookie& cookie,
    net::CookieStore::ChangeCause change_cause) {
  for (base::LinkNode<Listener>* node = listeners_.head();
       node != listeners_.end(); node = node->next()) {
    node->value()->DispatchCookieChange(cookie, change_cause);
  }
}

}  // namespace network
