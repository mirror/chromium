// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/threading/platform_thread.h"
#include "build/build_config.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/browser/webrtc/webrtc_content_browsertest_base.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/public/test/test_utils.h"

namespace content {

#if defined(OS_ANDROID) && defined(ADDRESS_SANITIZER)
// Renderer crashes under Android ASAN: https://crbug.com/408496.
#define MAYBE_WebRtcPausePlayBrowserTest DISABLED_WebRtcPausePlayBrowserTest
#else
#define MAYBE_WebRtcPausePlayBrowserTest WebRtcPausePlayBrowserTest
#endif

class MAYBE_WebRtcPausePlayBrowserTest : public WebRtcContentBrowserTestBase {
 public:
  MAYBE_WebRtcPausePlayBrowserTest() {}
  ~MAYBE_WebRtcPausePlayBrowserTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    WebRtcContentBrowserTestBase::SetUpCommandLine(command_line);
    // Automatically grant device permission.
    AppendUseFakeUIForMediaStreamFlag();
  }

 protected:
  void MakeTypicalPeerConnectionCall(const std::string& javascript) {
    MakeTypicalCall(javascript, "/media/peerconnection-pause-play.html");
  }
};

IN_PROC_BROWSER_TEST_F(MAYBE_WebRtcPausePlayBrowserTest,
                       Survives10SecondsWith10PeerConnectionPausePlaying) {
  // Args: runtimeSeconds, numPeerConnections, pausePlayIterationDelayMillis,
  // elementType.
  MakeTypicalPeerConnectionCall("runPausePlayTest(10, 10, 100, 'video');");
}

}  // namespace content
