// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/network/restricted_cookie_change_nexus.h"

#include "base/bind.h"
#include "base/no_destructor.h"

namespace content {

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
  LOG(ERROR) << __func__ << " called";

  if (!cookie.IncludeForRequestURL(url_, options_))
    return;

  callback_.Run(cookie, change_cause);
}

RestrictedCookieChangeNexus::ListenerReference::ListenerReference() = default;
RestrictedCookieChangeNexus::ListenerReference::~ListenerReference() {
  if (!listener_node_)
    return;

  listener_node_->nexus()->RemoveListener(listener_node_.get());
}

RestrictedCookieChangeNexus::ListenerReference::ListenerReference(
    ListenerReference&&) noexcept = default;

RestrictedCookieChangeNexus::ListenerReference&
RestrictedCookieChangeNexus::ListenerReference::operator=(
    ListenerReference&&) noexcept = default;

RestrictedCookieChangeNexus::ListenerReference::ListenerReference(
    std::unique_ptr<Listener> listener_node) noexcept
    : listener_node_(std::move(listener_node)) {}

// static
RestrictedCookieChangeNexus* RestrictedCookieChangeNexus::Instance(
    net::CookieStore* cookie_store) {
  base::NoDestructor<RestrictedCookieChangeNexus> singleton_instance(
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
  LOG(ERROR) << __func__ << " called";
  return ListenerReference(std::unique_ptr<Listener>(
      new Listener(this, url, options, std::move(callback))));
}

RestrictedCookieChangeNexus::RestrictedCookieChangeNexus(
    net::CookieStore* cookie_store)
    : cookie_store_(cookie_store) {
  LOG(ERROR) << __func__ << " called";
  // Using base::Unretained is safe because RestrictedCookieChangeNexus
  // instances are never destroyed.
  cookie_store_->AddCallbackForAllChanges(
      base::BindRepeating(&RestrictedCookieChangeNexus::OnCookieStoreChange,
                          base::Unretained(this)));
}

void RestrictedCookieChangeNexus::RemoveListener(
    base::LinkNode<Listener>* listener_node) {
  LOG(ERROR) << __func__ << " called";
  listener_node->RemoveFromList();
}

void RestrictedCookieChangeNexus::OnCookieStoreChange(
    const net::CanonicalCookie& cookie,
    net::CookieStore::ChangeCause change_cause) {
  LOG(ERROR) << __func__ << " called";
  for (base::LinkNode<Listener>* node = listeners_.head();
       node != listeners_.end(); node = node->next()) {
    node->value()->DispatchCookieChange(cookie, change_cause);
  }
}

}  // namespace content