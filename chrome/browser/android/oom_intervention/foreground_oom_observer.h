// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_OOM_INTERVENTION_FOREGROUND_OOM_OBSERVER_H_
#define CHROME_BROWSER_ANDROID_OOM_INTERVENTION_FOREGROUND_OOM_OBSERVER_H_

#include "chrome/browser/metrics/oom/out_of_memory_reporter.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

class ForegroundOomObserver
    : public content::WebContentsObserver,
      public content::WebContentsUserData<ForegroundOomObserver>,
      public OutOfMemoryReporter::Observer {
 public:
  ~ForegroundOomObserver() override = default;

 private:
  friend class content::WebContentsUserData<ForegroundOomObserver>;

  explicit ForegroundOomObserver(content::WebContents* web_contents);

  // content::WebContentsObserver:
  void WebContentsDestroyed() override;

  // OutOfMemoryReporter::Observer:
  void OnForegroundOOMDetected(const GURL& url,
                               ukm::SourceId source_id) override;
};

#endif  // CHROME_BROWSER_ANDROID_OOM_INTERVENTION_FOREGROUND_OOM_OBSERVER_H_
