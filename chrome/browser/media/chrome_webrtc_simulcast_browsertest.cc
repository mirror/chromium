// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "chrome/browser/media/webrtc_browsertest_base.h"
#include "chrome/browser/media/webrtc_browsertest_common.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_tabstrip.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/test/browser_test_utils.h"
#include "media/base/media_switches.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/perf/perf_test.h"
#include "ui/gl/gl_switches.h"

static const char kSimulcastTestPage[] = "/webrtc/webrtc-simulcast.html";

// Simulcast integration test. This test ensures 'a=x-google-flag:conference'
// is working and that Chrome is capable of sending simulcast streams. To run
// this test, Chrome must be hideout-enabled.
class WebRtcSimulcastBrowserTest : public WebRtcTestBase {
 public:
  // TODO(phoglund): Make it possible to enable DetectErrorsInJavaScript() here.

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // Just answer 'allow' to GetUserMedia invocations.
    command_line->AppendSwitch(switches::kUseFakeUIForMediaStream);

    // Use fake devices in order to run on VMs.
    command_line->AppendSwitch(switches::kUseFakeDeviceForMediaStream);
  }
};

IN_PROC_BROWSER_TEST_F(WebRtcSimulcastBrowserTest,
                       TestVgaReturnsTwoSimulcastStreams) {
  ASSERT_TRUE(embedded_test_server()->InitializeAndWaitUntilReady());

  ui_test_utils::NavigateToURL(
      browser(), embedded_test_server()->GetURL(kSimulcastTestPage));

  content::WebContents* tab_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  ASSERT_EQ("OK", ExecuteJavascript("testVgaReturnsTwoSimulcastStreams()",
                                    tab_contents));
}
