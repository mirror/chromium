// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_AW_BROWSER_MAIN_PARTS_H_
#define ANDROID_WEBVIEW_BROWSER_AW_BROWSER_MAIN_PARTS_H_

#include <memory>

#include "android_webview/browser/android_webview_field_trials.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/metrics/field_trial.h"
#include "components/prefs/pref_service.h"
#include "components/variations/entropy_provider.h"
#include "components/variations/service/variations_field_trial_creator.h"
#include "content/public/browser/browser_main_parts.h"

namespace base {
class MessageLoop;
}

namespace android_webview {

class AwContentBrowserClient;

class AwBrowserMainParts : public content::BrowserMainParts {
 public:
  explicit AwBrowserMainParts(AwContentBrowserClient* browser_client);
  ~AwBrowserMainParts() override;

  // Overriding methods from content::BrowserMainParts.
  void PreEarlyInitialization() override;
  int PreCreateThreads() override;
  void PreMainMessageLoopRun() override;
  bool MainMessageLoopRun(int* result_code) override;
  void SetUpFieldTrials();

 private:
  // Android specific UI MessageLoop.
  std::unique_ptr<base::MessageLoop> main_message_loop_;

  AwContentBrowserClient* browser_client_;

  // Statistical testing infrastructure for the entire application. empty until
  // |SetupFieldTrials()| is called.
  std::unique_ptr<base::FieldTrialList> field_trial_list_;

  // Members initialized in PreMainMessageLoopRun, needed in
  // PreMainMessageLoopRunThreadsCreated.
  base::FilePath user_data_dir_;

  // Platform specific field trial infrastructure
  variations::AndroidWebViewFieldTrials webview_field_trials_;

  // Member responsible for creating a feature list from the seed
  std::unique_ptr<variations::VariationsFieldTrialCreator>
      variations_field_trial_creator_;

  DISALLOW_COPY_AND_ASSIGN(AwBrowserMainParts);
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_AW_BROWSER_MAIN_PARTS_H_
