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
      callback_(std::move(callback)) {
  LOG(ERROR) << __func__ << " url: " << url;
  LOG(ERROR) << __func__ << " url_: " << url_;
  LOG(ERROR) << __func__ << " this: " << this;
}

void RestrictedCookieChangeNexus::Listener::DispatchCookieChange(
    const net::CanonicalCookie& cookie,
    net::CookieStore::ChangeCause change_cause) {
  LOG(ERROR) << __func__ << " url: " << url_;
  LOG(ERROR) << __func__ << " this: " << this;

  if (!cookie.IncludeForRequestURL(url_, options_))
    return;

  callback_.Run(cookie, change_cause);
}

RestrictedCookieChangeNexus::ListenerReference::ListenerReference() = default;
RestrictedCookieChangeNexus::ListenerReference::~ListenerReference() {
  LOG(ERROR) << __func__ << " listener_node_: " << listener_.get();
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
    : listener_(std::move(listener)) {
  LOG(ERROR) << __func__ << " listener_: " << listener_.get();
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
  ListenerReference listener_reference =
      ListenerReference(std::unique_ptr<Listener>(
          new Listener(this, url, options, std::move(callback))));
  listeners_.Append(listener_reference.listener());
  return listener_reference;
}

RestrictedCookieChangeNexus::RestrictedCookieChangeNexus(
    net::CookieStore* cookie_store)
    : cookie_store_(cookie_store) {
  // Using base::Unretained is safe because RestrictedCookieChangeNexus
  // instances are never destroyed.
  cookie_store_subscription_ = cookie_store_->AddCallbackForAllChanges(
      base::BindRepeating(&RestrictedCookieChangeNexus::OnCookieStoreChange,
                          base::Unretained(this)));
  LOG(ERROR) << __func__ << " head: " << listeners_.head()
             << " tail: " << listeners_.tail();
}

void RestrictedCookieChangeNexus::RemoveListener(
    std::unique_ptr<Listener> listener) {
  LOG(ERROR) << __func__ << " listener: " << listener.get();
  listener->RemoveFromList();
}

void RestrictedCookieChangeNexus::OnCookieStoreChange(
    const net::CanonicalCookie& cookie,
    net::CookieStore::ChangeCause change_cause) {
  LOG(ERROR) << __func__ << " end: " << listeners_.end();
  for (base::LinkNode<Listener>* node = listeners_.head();
       node != listeners_.end(); node = node->next()) {
    LOG(ERROR) << __func__ << " node: " << node;
    node->value()->DispatchCookieChange(cookie, change_cause);
  }
}

}  // namespace network
