// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/net/cookies/ns_http_system_cookie_store.h"

#include "base/bind.h"
#include "base/time/time.h"

#import "ios/net/cookies/cookie_creation_time_manager.h"
#import "ios/net/cookies/cookie_store_ios_client.h"
#import "net/base/mac/url_conversions.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace net {

// private
void DoCookieCallback(base::OnceClosure callback) {
  CookieStoreIOSClient* client = net::GetCookieStoreIOSClient();
  DCHECK(client);
  auto sequenced_task_runner = client->GetTaskRunner();
  DCHECK(sequenced_task_runner);
  sequenced_task_runner.get()->PostTask(FROM_HERE, std::move(callback));
}

#pragma mark -
#pragma mark NSHTTPSystemCookieStore methods

NSHTTPSystemCookieStore::NSHTTPSystemCookieStore()
    : cookie_store_([NSHTTPCookieStorage sharedHTTPCookieStorage]) {}

NSHTTPSystemCookieStore::NSHTTPSystemCookieStore(
    NSHTTPCookieStorage* cookie_store)
    : cookie_store_(cookie_store) {}

NSHTTPSystemCookieStore::~NSHTTPSystemCookieStore() = default;

NSArray* NSHTTPSystemCookieStore::GetCookiesForURL(const GURL& url) {
  NSArray* cookies = [cookie_store_ cookiesForURL:NSURLWithGURL(url)];
  // Sort cookies by decreasing path length, then creation time, as per
  // RFC6265.
  return [cookies sortedArrayUsingFunction:CompareCookies
                                   context:creation_time_manager_.get()];
}

NSArray* NSHTTPSystemCookieStore::GetAllCookies() {
  NSArray* cookies = cookie_store_.cookies;
  return [cookies sortedArrayUsingFunction:CompareCookies
                                   context:creation_time_manager_.get()];
}

void NSHTTPSystemCookieStore::DeleteCookie(NSHTTPCookie* cookie) {
  [cookie_store_ deleteCookie:cookie];
  creation_time_manager_->DeleteCreationTime(cookie);
}

void NSHTTPSystemCookieStore::SetCookie(
    NSHTTPCookie* cookie,
    const base::Time& optional_creation_time) {
  [cookie_store_ setCookie:cookie];
  base::Time cookie_time = optional_creation_time;
  if (cookie_time.is_null()) {
    cookie_time = base::Time::Now();
  }

  creation_time_manager_->SetCreationTime(
      cookie, creation_time_manager_->MakeUniqueCreationTime(cookie_time));
}

void NSHTTPSystemCookieStore::ClearStore() {
  NSArray* copy = [cookie_store_.cookies copy];
  for (NSHTTPCookie* cookie in copy)
    [cookie_store_ deleteCookie:cookie];
  DCHECK_EQ(0u, cookie_store_.cookies.count);
  creation_time_manager_->Clear();
}

#pragma mark -
#pragma mark SystemCookieStore methods

void NSHTTPSystemCookieStore::GetCookiesForURLAsync(
    const GURL& url,
    SystemCookieCallbackForCookies callback) {
  NSArray* cookies = GetCookiesForURL(url);
  DoCookieCallback(base::BindOnce(std::move(callback), cookies));
}
void NSHTTPSystemCookieStore::GetAllCookiesAsync(
    SystemCookieCallbackForCookies callback) {
  NSArray* cookies = GetAllCookies();
  DoCookieCallback(base::BindOnce(std::move(callback), cookies));
}
void NSHTTPSystemCookieStore::DeleteCookieAsync(NSHTTPCookie* cookie,
                                                SystemCookieCallback callback) {
  DeleteCookie(cookie);
  DoCookieCallback(std::move(callback));
}

void NSHTTPSystemCookieStore::SetCookieAsync(
    NSHTTPCookie* cookie,
    const base::Time& optional_creation_time,
    SystemCookieCallback callback) {
  SetCookie(cookie, optional_creation_time);
  DoCookieCallback(std::move(callback));
}

void NSHTTPSystemCookieStore::ClearStoreAsync(SystemCookieCallback callback) {
  ClearStore();
  DoCookieCallback(std::move(callback));
}

NSHTTPCookieAcceptPolicy NSHTTPSystemCookieStore::GetCookieAcceptPolicy() {
  return [cookie_store_ cookieAcceptPolicy];
}

}  // namespace net
