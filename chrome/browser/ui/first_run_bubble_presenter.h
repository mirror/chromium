// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_FIRST_RUN_BUBBLE_PRESENTER_H_
#define CHROME_BROWSER_UI_FIRST_RUN_BUBBLE_PRESENTER_H_

#include "base/callback.h"
#include "base/macros.h"
#include "chrome/browser/ui/browser.h"
#include "components/search_engines/template_url_service_observer.h"
#include "components/sessions/core/session_id.h"

class TemplateURLService;

class FirstRunBubblePresenter : public TemplateURLServiceObserver {
 public:
  static void PresentWhenReady(Browser* browser);

 private:
  FirstRunBubblePresenter(TemplateURLService* url_service, Browser* browser);

  ~FirstRunBubblePresenter() override;

  // TemplateURLServiceObserver:
  void OnTemplateURLServiceChanged() override;
  void OnTemplateURLServiceShuttingDown() override;

  TemplateURLService* template_url_service_;
  SessionID session_id_;

  DISALLOW_COPY_AND_ASSIGN(FirstRunBubblePresenter);
};

#endif  // CHROME_BROWSER_UI_FIRST_RUN_BUBBLE_PRESENTER_H_
