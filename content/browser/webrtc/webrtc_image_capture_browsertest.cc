// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "build/build_config.h"
#include "content/browser/webrtc/webrtc_webcam_browsertest.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "content/shell/browser/shell.h"
#include "media/base/media_switches.h"
#include "media/capture/video/fake_video_capture_device_factory.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "services/video_capture/public/cpp/constants.h"

#if defined(OS_ANDROID)
#include "base/android/build_info.h"
#endif

namespace content {

#if defined(OS_WIN)
// These tests are flaky on WebRTC Windows bots: https://crbug.com/633242.
#define MAYBE_GetCapabilities DISABLED_GetCapabilities
#define MAYBE_TakePhoto DISABLED_TakePhoto
#define MAYBE_GrabFrame DISABLED_GrabFrame
#else
#define MAYBE_GetCapabilities GetCapabilities
#define MAYBE_TakePhoto TakePhoto
#define MAYBE_GrabFrame GrabFrame
#endif

namespace {

static const char kImageCaptureHtmlFile[] = "/media/image_capture_test.html";

// TODO(mcasas): enable real-camera tests by disabling the Fake Device for
// platforms where the ImageCaptureCode is landed, https://crbug.com/656810
static struct TargetCamera {
  bool use_fake;
  bool use_video_capture_service;
} const kTestParameters[] = {{true, false},
// Mojo video capture is currently not supported on Android
// TODO(chfremer): Remove this as soon as https://crbug.com/720500 is
// resolved.
#if !defined(OS_ANDROID)
                             {true, true},
#endif
#if defined(OS_LINUX) || defined(OS_MACOSX) || defined(OS_ANDROID)
                             {false, false},
                             {false, true}
#endif
};

}  // namespace

// This class is the content_browsertests for Image Capture API, which allows
// for capturing still images out of a MediaStreamTrack. Is a
// WebRtcWebcamBrowserTest to be able to use a physical camera.
class WebRtcImageCaptureBrowserTestBase : public WebRtcWebcamBrowserTest {
 public:
  WebRtcImageCaptureBrowserTestBase() = default;
  ~WebRtcImageCaptureBrowserTestBase() override = default;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    WebRtcWebcamBrowserTest::SetUpCommandLine(command_line);

    ASSERT_FALSE(base::CommandLine::ForCurrentProcess()->HasSwitch(
        switches::kUseFakeDeviceForMediaStream));

    // "GetUserMedia": enables navigator.mediaDevices.getUserMedia();
    // TODO(mcasas): remove GetUserMedia after https://crbug.com/503227.
    // "ImageCapture": enables the ImageCapture API.
    // TODO(mcasas): remove ImageCapture after https://crbug.com/603328.
    base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
        switches::kEnableBlinkFeatures, "GetUserMedia,ImageCapture");
  }

  void SetUp() override {
    ASSERT_TRUE(embedded_test_server()->InitializeAndListen());
    WebRtcWebcamBrowserTest::SetUp();
  }

  // Tries to run a |command| JS test, returning true if the test can be safely
  // skipped or it works as intended, or false otherwise.
  bool RunImageCaptureTestCase(const std::string& command) {
#if defined(OS_ANDROID)
    // TODO(mcasas): fails on Lollipop devices: https://crbug.com/634811
    if (base::android::BuildInfo::GetInstance()->sdk_int() <
        base::android::SDK_VERSION_MARSHMALLOW) {
      return true;
    }
#endif

    GURL url(embedded_test_server()->GetURL(kImageCaptureHtmlFile));
    NavigateToURL(shell(), url);

    if (!IsWebcamAvailableOnSystem(shell()->web_contents())) {
      DVLOG(1) << "No video device; skipping test...";
      return true;
    }

    std::string result;
    if (!ExecuteScriptAndExtractString(shell(), command, &result))
      return false;
    DLOG_IF(ERROR, result != "OK") << result;
    return result == "OK";
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(WebRtcImageCaptureBrowserTestBase);
};

// Test fixture for setting up a fake device that successfully serves all
// image capture requests.
class WebRtcImageCaptureSucceedsBrowserTest
    : public WebRtcImageCaptureBrowserTestBase,
      public testing::WithParamInterface<struct TargetCamera> {
 public:
  WebRtcImageCaptureSucceedsBrowserTest() = default;
  ~WebRtcImageCaptureSucceedsBrowserTest() override = default;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    WebRtcImageCaptureBrowserTestBase::SetUpCommandLine(command_line);

    if (GetParam().use_fake) {
      base::CommandLine::ForCurrentProcess()->AppendSwitch(
          switches::kUseFakeDeviceForMediaStream);
      ASSERT_TRUE(base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kUseFakeDeviceForMediaStream));
    }
    if (GetParam().use_video_capture_service) {
      base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
          switches::kEnableFeatures, video_capture::kMojoVideoCapture.name);
    }
  }
};

IN_PROC_BROWSER_TEST_P(WebRtcImageCaptureSucceedsBrowserTest,
                       MAYBE_GetCapabilities) {
  embedded_test_server()->StartAcceptingConnections();
  ASSERT_TRUE(
      RunImageCaptureTestCase("testCreateAndGetCapabilitiesSucceeds()"));
}

IN_PROC_BROWSER_TEST_P(WebRtcImageCaptureSucceedsBrowserTest, MAYBE_TakePhoto) {
  embedded_test_server()->StartAcceptingConnections();
  ASSERT_TRUE(RunImageCaptureTestCase("testCreateAndTakePhotoSucceeds()"));
}

IN_PROC_BROWSER_TEST_P(WebRtcImageCaptureSucceedsBrowserTest, MAYBE_GrabFrame) {
  embedded_test_server()->StartAcceptingConnections();
  ASSERT_TRUE(RunImageCaptureTestCase("testCreateAndGrabFrameSucceeds()"));
}

INSTANTIATE_TEST_CASE_P(,
                        WebRtcImageCaptureSucceedsBrowserTest,
                        testing::ValuesIn(kTestParameters));

// Test fixture for setting up a fake device that responds to invocation of
// GetPhotoCapabilities with a failure response.
class WebRtcImageCaptureGetCapabilitiesFailsBrowserTest
    : public WebRtcImageCaptureBrowserTestBase,
      public testing::WithParamInterface<bool /*use video capture service*/> {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    WebRtcImageCaptureBrowserTestBase::SetUpCommandLine(command_line);

    base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
        switches::kUseFakeDeviceForMediaStream,
        std::string("config=") + media::FakeVideoCaptureDeviceFactory::
                                     kDeviceConfigForGetPhotoCapabilitiesFails);
    if (GetParam()) {
      base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
          switches::kEnableFeatures, video_capture::kMojoVideoCapture.name);
    }
  }
};

IN_PROC_BROWSER_TEST_P(WebRtcImageCaptureGetCapabilitiesFailsBrowserTest,
                       GetCapabilities) {
  embedded_test_server()->StartAcceptingConnections();
  // With the fake device reporting a failure, we expect an empty set of
  // capabilities to reported back to JS.
  ASSERT_TRUE(
      RunImageCaptureTestCase("testCreateAndGetCapabilitiesSucceeds()"));
}

IN_PROC_BROWSER_TEST_P(WebRtcImageCaptureGetCapabilitiesFailsBrowserTest,
                       TakePhoto) {
  embedded_test_server()->StartAcceptingConnections();
  ASSERT_TRUE(RunImageCaptureTestCase("testCreateAndTakePhotoSucceeds()"));
}

IN_PROC_BROWSER_TEST_P(WebRtcImageCaptureGetCapabilitiesFailsBrowserTest,
                       GrabFrame) {
  embedded_test_server()->StartAcceptingConnections();
  ASSERT_TRUE(RunImageCaptureTestCase("testCreateAndGrabFrameSucceeds()"));
}

INSTANTIATE_TEST_CASE_P(,
                        WebRtcImageCaptureGetCapabilitiesFailsBrowserTest,
                        testing::Bool());

// Test fixture for setting up a fake device that responds to invocation of
// SetPhotoOptions with a failure response.
class WebRtcImageCaptureSetOptionsFailsBrowserTest
    : public WebRtcImageCaptureBrowserTestBase,
      public testing::WithParamInterface<bool /*use video capture service*/> {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    WebRtcImageCaptureBrowserTestBase::SetUpCommandLine(command_line);

    base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
        switches::kUseFakeDeviceForMediaStream,
        std::string("config=") + media::FakeVideoCaptureDeviceFactory::
                                     kDeviceConfigForSetPhotoOptionsFails);
    if (GetParam()) {
      base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
          switches::kEnableFeatures, video_capture::kMojoVideoCapture.name);
    }
  }
};

IN_PROC_BROWSER_TEST_P(WebRtcImageCaptureSetOptionsFailsBrowserTest,
                       TakePhoto) {
  embedded_test_server()->StartAcceptingConnections();
  ASSERT_TRUE(RunImageCaptureTestCase("testCreateAndTakePhotoIsRejected()"));
}

IN_PROC_BROWSER_TEST_P(WebRtcImageCaptureSetOptionsFailsBrowserTest,
                       GrabFrame) {
  embedded_test_server()->StartAcceptingConnections();
  ASSERT_TRUE(RunImageCaptureTestCase("testCreateAndGrabFrameSucceeds()"));
}

INSTANTIATE_TEST_CASE_P(,
                        WebRtcImageCaptureSetOptionsFailsBrowserTest,
                        testing::Bool());

// Test fixture for setting up a fake device that responds to invocation of
// TakePhoto with a failure response.
class WebRtcImageCaptureTakePhotoFailsBrowserTest
    : public WebRtcImageCaptureBrowserTestBase,
      public testing::WithParamInterface<bool /*use video capture service*/> {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    WebRtcImageCaptureBrowserTestBase::SetUpCommandLine(command_line);

    base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
        switches::kUseFakeDeviceForMediaStream,
        std::string("config=") + media::FakeVideoCaptureDeviceFactory::
                                     kDeviceConfigForTakePhotoFails);
    if (GetParam()) {
      base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
          switches::kEnableFeatures, video_capture::kMojoVideoCapture.name);
    }
  }
};

IN_PROC_BROWSER_TEST_P(WebRtcImageCaptureTakePhotoFailsBrowserTest, TakePhoto) {
  embedded_test_server()->StartAcceptingConnections();
  ASSERT_TRUE(RunImageCaptureTestCase("testCreateAndTakePhotoIsRejected()"));
}

IN_PROC_BROWSER_TEST_P(WebRtcImageCaptureTakePhotoFailsBrowserTest, GrabFrame) {
  embedded_test_server()->StartAcceptingConnections();
  ASSERT_TRUE(RunImageCaptureTestCase("testCreateAndGrabFrameSucceeds()"));
}

INSTANTIATE_TEST_CASE_P(,
                        WebRtcImageCaptureTakePhotoFailsBrowserTest,
                        testing::Bool());

}  // namespace content
