// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_FIRST_RUN_BUBBLE_PRESENTER_H_
#define CHROME_BROWSER_UI_FIRST_RUN_BUBBLE_PRESENTER_H_

#include "base/callback.h"
#include "chrome/browser/profiles/profile.h"
#include "components/search_engines/template_url_service.h"
#include "components/search_engines/template_url_service_observer.h"

class FirstRunBubblePresenter : public TemplateURLServiceObserver {
 public:
  using PresentCallback = base::Closure;
  static void PresentWhenReady(Profile* profile,
                               const PresentCallback& callback);

 private:
  explicit FirstRunBubblePresenter(TemplateURLService* url_service,
                                   const PresentCallback& present_callback);

  ~FirstRunBubblePresenter() override;

  // TemplateURLServiceObserver:
  void OnTemplateURLServiceChanged() override;

  TemplateURLService* template_url_service_;
  PresentCallback present_callback_;
};

#endif  // CHROME_BROWSER_UI_FIRST_RUN_BUBBLE_PRESENTER_H_
