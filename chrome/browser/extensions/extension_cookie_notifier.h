// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_EXTENSION_COOKIE_NOTIFIER_H_
#define CHROME_BROWSER_EXTENSIONS_EXTENSION_COOKIE_NOTIFIER_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/profiles/profile_io_data.h"
#include "net/cookies/cookie_store.h"

class Profile;

// Sends cookie-change notifications from the UI thread via
// chrome::NOTIFICATION_COOKIE_CHANGED_FOR_EXTENSIONS.
class ExtensionCookieNotifier : public ProfileIOData::CookieNotifier {
 public:
  explicit ExtensionCookieNotifier(Profile* profile);
  ~ExtensionCookieNotifier() override;

  // ProfileIOData::CookieNotifier
  void OnCookieChanged(const net::CanonicalCookie& cookie,
                       net::CookieStore::ChangeCause cause) override;

 private:
  void OnCookieChangedAsyncHelper(const net::CanonicalCookie& cookie,
                                  bool removed,
                                  net::CookieStore::ChangeCause cause);

  const base::Callback<Profile*(void)> profile_getter_;
  base::WeakPtrFactory<ExtensionCookieNotifier> factory_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionCookieNotifier);
};

#endif  // CHROME_BROWSER_EXTENSIONS_EXTENSION_COOKIE_NOTIFIER_H_
