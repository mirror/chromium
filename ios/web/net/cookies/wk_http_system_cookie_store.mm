// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/net/cookies/wk_http_system_cookie_store.h"

#include "base/bind.h"
#include "base/ios/ios_util.h"
#import "base/mac/bind_objc_block.h"
#import "ios/net/cookies/cookie_creation_time_manager.h"
#include "ios/web/public/web_thread.h"
#import "net/base/mac/url_conversions.h"
#include "url/gurl.h"

#if defined(__IPHONE_11_0) && (__IPHONE_OS_VERSION_MAX_ALLOWED >= __IPHONE_11_0)

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace web {

WKHTTPSystemCookieStore::WKHTTPSystemCookieStore(
    WKHTTPCookieStore* cookie_store)
    : cookie_store_(cookie_store) {}

WKHTTPSystemCookieStore::~WKHTTPSystemCookieStore() = default;

#pragma mark -
#pragma mark SystemCookieStore methods

void WKHTTPSystemCookieStore::GetCookiesForURLAsync(
    const GURL& url,
    SystemCookieCallbackForCookies callback) {
  __block SystemCookieCallbackForCookies shared_callback = std::move(callback);
  NSURL* ns_url = net::NSURLWithGURL(url);
  [cookie_store_ getAllCookies:^(NSArray<NSHTTPCookie*>* cookies) {
    NSMutableArray* result = [NSMutableArray array];
    for (NSHTTPCookie* cookie in cookies) {
      if ([ns_url isEqual:cookie.properties[NSHTTPCookieOriginURL]]) {
        [result addObject:cookie];
      }
    }
    [result sortUsingFunction:net::SystemCookieStore::CompareCookies
                      context:creation_time_manager_.get()];
    // WKHTTPCookieStore executes callbacks on the main thread, while
    // SystemCookieStore operates on IO thread.
    web::WebThread::PostTask(
        web::WebThread::IO, FROM_HERE,
        base::BindOnce(std::move(shared_callback), result));
  }];
}

void WKHTTPSystemCookieStore::GetAllCookiesAsync(
    SystemCookieCallbackForCookies callback) {
  __block SystemCookieCallbackForCookies shared_callback = std::move(callback);

  [cookie_store_ getAllCookies:^(NSArray<NSHTTPCookie*>* cookies) {
    NSArray* input = cookies;
    NSArray* result =
        [input sortedArrayUsingFunction:net::SystemCookieStore::CompareCookies
                                context:creation_time_manager_.get()];
    // WKHTTPCookieStore executes callbacks on the main thread, while
    // SystemCookieStore operates on IO thread.
    web::WebThread::PostTask(
        web::WebThread::IO, FROM_HERE,
        base::BindOnce(std::move(shared_callback), result));
  }];
}

void WKHTTPSystemCookieStore::DeleteCookieAsync(NSHTTPCookie* cookie,
                                                SystemCookieCallback callback) {
  __block SystemCookieCallback shared_callback = std::move(callback);

  [cookie_store_ deleteCookie:(NSHTTPCookie*)cookie
            completionHandler:^{
              creation_time_manager_->DeleteCreationTime(cookie);
              // WKHTTPCookieStore calls callbacks on the main thread, while
              // cookie store operates on IO thread.
              web::WebThread::PostTask(web::WebThread::IO, FROM_HERE,
                                       std::move(shared_callback));
            }];
}

void WKHTTPSystemCookieStore::SetCookieAsync(
    NSHTTPCookie* cookie,
    const base::Time& optional_creation_time,

    SystemCookieCallback callback) {
  __block SystemCookieCallback shared_callback = std::move(callback);

  [cookie_store_ setCookie:cookie
         completionHandler:^{
           // Set creation time as soon as possible
           base::Time cookie_time = optional_creation_time;
           if (cookie_time.is_null()) {
             cookie_time = base::Time::Now();
           }
           creation_time_manager_->SetCreationTime(
               cookie,
               creation_time_manager_->MakeUniqueCreationTime(cookie_time));
           // WKHTTPCookieStore calls callbacks on the main thread, while cookie
           // store operates on IO thread.
           web::WebThread::PostTask(web::WebThread::IO, FROM_HERE,
                                    std::move(shared_callback));

         }];
}

void WKHTTPSystemCookieStore::ClearStoreAsync(SystemCookieCallback callback) {
  __block SystemCookieCallback shared_callback = std::move(callback);

  [cookie_store_ getAllCookies:^(NSArray<NSHTTPCookie*>* cookies) {
    for (NSHTTPCookie* cookie in cookies) {
      [cookie_store_ deleteCookie:(NSHTTPCookie*)cookie completionHandler:nil];
    }
    creation_time_manager_->Clear();
    // WKHTTPCookieStore calls callbacks on the main thread, while cookie
    // store operates on IO thread.
    web::WebThread::PostTask(web::WebThread::IO, FROM_HERE,
                             std::move(shared_callback));
  }];
}

NSHTTPCookieAcceptPolicy WKHTTPSystemCookieStore::GetCookieAcceptPolicy() {
  // TODO(crbug.com/759226): Update this with the correct CookieAcceptPolicy.
  return NSHTTPCookieAcceptPolicyAlways;
}

}  // namespace web

#endif
