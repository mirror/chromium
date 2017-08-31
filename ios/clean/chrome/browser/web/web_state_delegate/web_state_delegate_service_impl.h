// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_WEB_WEB_STATE_DELEGATE_WEB_STATE_DELEGATE_SERVICE_IMPL_H_
#define IOS_CLEAN_CHROME_BROWSER_WEB_WEB_STATE_DELEGATE_WEB_STATE_DELEGATE_SERVICE_IMPL_H_

#import "ios/chrome/browser/ui/browser_list/browser_list_observer.h"
#import "ios/clean/chrome/browser/web/web_state_delegate/web_state_delegate_service.h"

class BrowserList;

// Concrete subclass of WebStateDelegateService.
class WebStateDelegateServiceImpl : public BrowserListObserver,
                                    public WebStateDelegateService {
 public:
  // Constructor for an WebStateDelegateService that supplies delegates to
  // WebStates in |browser_list|.
  WebStateDelegateServiceImpl(BrowserList* browser_list);
  ~WebStateDelegateServiceImpl() override;

 private:
  // BrowserListObserver:
  void OnBrowserCreated(BrowserList* browser_list, Browser* browser) override;
  void OnBrowserRemoved(BrowserList* browser_list, Browser* browser) override;

  // KeyedService:
  void Shutdown() override;

  // WebStateDelegateService:
  void AttachWebStateDelegates() override;
  void DetachWebStateDelegates() override;

  // Attaches or detaches a WebStateDelegate for every WebState in |browser|.
  void AttachWebStateDelegate(Browser* browser);
  void DetachWebStateDelegate(Browser* browser);

  // The BrowserList passed to constructor.
  BrowserList* browser_list_;
  // Whether the delegates are attached.
  bool attached_;

  DISALLOW_COPY_AND_ASSIGN(WebStateDelegateServiceImpl);
};

#endif  // IOS_CLEAN_CHROME_BROWSER_WEB_WEB_STATE_DELEGATE_WEB_STATE_DELEGATE_SERVICE_IMPL_H_
