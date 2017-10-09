// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_INSTALLABLE_INSTALLABLE_PARAMS_H_
#define CHROME_BROWSER_INSTALLABLE_INSTALLABLE_PARAMS_H_

// This struct specifies the work to be done by the InstallableManager.
// Data is cached and fetched in the order specified in this struct. A web app
// manifest will always be fetched first.
struct InstallableParams {
  enum ServiceWorkerWaitBehavior {
    WAIT_INDEFINITELY,
    CHECK_IMMEDIATELY,
  };

  // Check whether there is a fetchable, non-empty icon in the manifest
  // conforming to the primary icon size parameters.
  bool valid_primary_icon = false;

  // Check whether there is a fetchable, non-empty icon in the manifest
  // conforming to the badge icon size parameters.
  bool valid_badge_icon = false;

  // Check whether the site has a manifest valid for a web app.
  bool valid_manifest = false;

  // Check whether the site has a service worker controlling the manifest start
  // URL and the current URL. Implies the |valid_manifest| check.
  bool has_worker = false;
};

#endif  // CHROME_BROWSER_INSTALLABLE_INSTALLABLE_PARAMS_H_
