// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_WEB_WEB_STATE_DELEGATE_WEB_STATE_DELEGATE_SERVICE_H_
#define IOS_CLEAN_CHROME_BROWSER_WEB_WEB_STATE_DELEGATE_WEB_STATE_DELEGATE_SERVICE_H_

#include "base/macros.h"
#include "components/keyed_service/core/keyed_service.h"

// WebStateDelegateService sets up delegates for the WebStates added to the
// Browsers in a BrowserState.
class WebStateDelegateService : public KeyedService {
 public:
  WebStateDelegateService() = default;
  ~WebStateDelegateService() override = default;

  // Instructs the service to create WebStateDelegates for the BrowserState's
  // WebStates.
  virtual void AttachWebStateDelegates() = 0;

  // Instructs the service to remove the WebStateDelegates from the
  // BrowserState's WebStates.
  virtual void DetachWebStateDelegates() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(WebStateDelegateService);
};

#endif  // IOS_CLEAN_CHROME_BROWSER_WEB_WEB_STATE_DELEGATE_WEB_STATE_DELEGATE_SERVICE_H_
