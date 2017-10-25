// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/base/in_process_browser_test.h"

#include "base/command_line.h"
#include "chrome/browser/ui/test/test_browser_dialog.h"
#include "chrome/common/chrome_switches.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "chrome/browser/chrome_notification_types.h"
#include "content/public/browser/notification_service.h"

class FirstRunInternalPosixTest : public DialogBrowserTest,                                    public content::NotificationObserver {
 public:
  FirstRunInternalPosixTest() {}

  // DialogBrowserTest:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(switches::kForceFirstRun);
    command_line->AppendSwitch(switches::kForceFirstRunDialog);
  }

  void SetUp() override {
    // The modal dialog will spawn and spin a nested RunLoop when
    // content::BrowserTestBase::SetUp() invokes content::ContentMain().
    // BrowserTestBase sets GetContentMainParams()->ui_task before this, but the
    // ui_task isn't actually Run() until after the dialog is spawned in
    // ChromeBrowserMainParts::PreMainMessageLoopRunImpl(). Instead, try to
    // inspect state by posting a task to run in that nested RunLoop.
    // There's no MessageLoop to enqueue a task on yet, but we know this first
    // run dialog needs a Profile, so wait for that to be created, and post the
    // task then.
    content::NotificationService::Create();
    registrar_.Add(this, chrome::NOTIFICATION_PROFILE_ADDED,
                   content::NotificationService::AllBrowserContextsAndSources());
    DialogBrowserTest::SetUp();  // InspectState() runs here.
  }

  void ShowDialog(const std::string& name) override {
    // By the time we get here, the dialog will already be showing. Pop a dummy
    // dialog?
  }

  // content::NotificationObserver:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override {
    EXPECT_TRUE(base::ThreadTaskRunnerHandle::Get());
    base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE, base::Bind(&FirstRunInternalPosixTest::InspectState, base::Unretained(this)));
  }

 private:
  void InspectState() {
    DLOG(INFO) << "INSPECT!";
  }

  content::NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(FirstRunInternalPosixTest);
};

IN_PROC_BROWSER_TEST_F(FirstRunInternalPosixTest, HandleSigterm) {
  DLOG(INFO) << "HERE?!?";
  RunDialog();
}
