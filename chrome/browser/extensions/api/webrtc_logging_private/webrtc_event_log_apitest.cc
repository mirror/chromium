// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <utility>

#include "base/command_line.h"
#include "base/files/file_path_watcher.h"
#include "base/json/json_writer.h"
#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/synchronization/waitable_event.h"
#include "base/test/simple_test_clock.h"
#include "base/threading/platform_thread.h"
#include "base/threading/thread_restrictions.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/browser/extensions/api/webrtc_logging_private/webrtc_logging_private_api.h"
#include "chrome/browser/extensions/extension_function_test_utils.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/media/webrtc/webrtc_browsertest_base.h"
#include "chrome/browser/media/webrtc/webrtc_browsertest_common.h"
#include "chrome/common/chrome_switches.h"
#include "content/browser/webrtc/webrtc_event_log_manager.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/browser_test_utils.h"
#include "extensions/browser/api_test_utils.h"
#include "extensions/common/extension_builder.h"
#include "media/base/media_switches.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

using content::WebRtcEventLogManager;
using extensions::WebrtcLoggingPrivateStartWebRtcEventLoggingFunction;
using extensions::WebrtcLoggingPrivateStopWebRtcEventLoggingFunction;

namespace utils = extension_function_test_utils;

namespace {

static const char kMainWebrtcTestHtmlPage[] = "/webrtc/webrtc_jsep01_test.html";

std::string ParamsToString(const base::ListValue& parameters) {
  std::string parameter_string;
  EXPECT_TRUE(base::JSONWriter::Write(parameters, &parameter_string));
  return parameter_string;
}

class FileWaiter : public base::RefCountedThreadSafe<FileWaiter> {
 public:
  explicit FileWaiter(const base::FilePath& path)
      : found_(false), path_(path) {}

  bool Start() {
    base::ScopedAllowBlockingForTesting allow_blocking;
    if (base::PathExists(path_)) {
      found_ = true;
      return true;
    } else {
      return watcher_.Watch(path_, false /* recursive */,
                            base::Bind(&FileWaiter::Callback, this));
    }
  }

  // Returns true if |path_| became available.
  bool WaitForFile() {
    if (!found_) {
      run_loop_.Run();
    }
    return found_;
  }

  // implements FilePathWatcher::Callback
  void Callback(const base::FilePath& path, bool error) {
    EXPECT_EQ(path, path_);
    if (!error)
      found_ = true;
    run_loop_.Quit();
  }

 private:
  friend class base::RefCountedThreadSafe<FileWaiter>;
  ~FileWaiter() {}
  base::RunLoop run_loop_;
  bool found_;
  base::FilePath path_;
  base::FilePathWatcher watcher_;
  DISALLOW_COPY_AND_ASSIGN(FileWaiter);
};

}  // namespace

class WebrtcEventLogApiTest : public WebRtcTestBase {
 protected:
  void SetUp() override {
    Initialize();
    WebRtcTestBase::SetUp();
    extension_ = extensions::ExtensionBuilder("Test").Build();
  }

  ~WebrtcEventLogApiTest() override {
    WebRtcEventLogManager::GetInstance()->InjectClockForTesting(nullptr);
  }

  // Needs to happen after WebRtcTestBase::SetUp(), but before the body of the
  // tests. Therefore, we call it directly from the tests, as the first step.
  void Initialize() {
    // We can't get the filename as a reply, because WebRtcEventLogManager is
    // hidden from us behind WebRTCEventLogHost. We therefore implicitly rely
    // on the filename being derived in a certain way.
    // TODO(eladalon): Remove this hack once WebRTCEventLogHost is eliminated,
    // by getting a reply from WebRtcEventLogManager with the exact filename.
    // https://crbug.com/775415
    base::Time frozen_time = base::Time::Now();
    frozen_clock_.SetNow(frozen_time);
    WebRtcEventLogManager::GetInstance()->InjectClockForTesting(&frozen_clock_);
  }

  void SetUpInProcessBrowserTestFixture() override {
    DetectErrorsInJavaScript();  // Look for errors in our rather complex js.
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    // Ensure the infobar is enabled, since we expect that in this test.
    EXPECT_FALSE(command_line->HasSwitch(switches::kUseFakeUIForMediaStream));

    // Always use fake devices.
    command_line->AppendSwitch(switches::kUseFakeDeviceForMediaStream);

    // Flag used by TestWebAudioMediaStream to force garbage collection.
    command_line->AppendSwitchASCII(switches::kJavaScriptFlags, "--expose-gc");

    // Enable the the event log in the extension API.
    command_line->AppendSwitch(
        switches::kEnableWebRtcEventLoggingFromExtension);
  }

  template <typename T>
  scoped_refptr<T> CreateExtensionFunction() {
    scoped_refptr<T> function(new T());
    function->set_extension(extension_.get());
    function->set_has_callback(true);
    return function;
  }

  void AppendTabIdAndUrl(base::ListValue* parameters,
                         content::WebContents* tab) {
    std::unique_ptr<base::DictionaryValue> request_info(
        new base::DictionaryValue());
    request_info->SetInteger("tabId",
                             extensions::ExtensionTabUtil::GetTabId(tab));
    parameters->Append(std::move(request_info));
    parameters->AppendString(tab->GetURL().GetOrigin().spec());
  }

  // Get the expected EventLog file name. The name will be
  // [temporary path]/[user_defined]_[date]_[time]_[pid]_[lid].log
  // /tmp/.org.chromium.Chromium.vsygNQ/dnFW8ch/Default/WebRTC
  // Logs/WebRtcEventLog_20171122_1811_1_1.log
  // TODO(eladalon): Remove this hack once WebRTCEventLogHost is eliminated,
  // by getting a reply from WebRtcEventLogManager with the exact filename.
  // https://crbug.com/775415
  base::FilePath GetExpectedEventLogFileName(const base::FilePath& base_file,
                                             int render_process_id) {
    static const int kExpectedPeerConnectionId = 1;

    base::Time::Exploded time;
    frozen_clock_.Now().LocalExplode(&time);
    char stamp[100];
    int written = base::snprintf(
        stamp, arraysize(stamp), "%04d%02d%02d_%02d%02d_%d_%d", time.year,
        time.month, time.day_of_month, time.hour, time.minute,
        render_process_id, kExpectedPeerConnectionId);
    CHECK(0 < written);
    CHECK(static_cast<size_t>(written) < arraysize(stamp));

    const std::string filename_suffix = std::string("_") + stamp;

    return base_file.AddExtension(FILE_PATH_LITERAL("log"))
        .InsertBeforeExtension(filename_suffix);
  }

 private:
  scoped_refptr<extensions::Extension> extension_;
  base::SimpleTestClock frozen_clock_;
};

IN_PROC_BROWSER_TEST_F(WebrtcEventLogApiTest, TestStartStopWebRtcEventLogging) {
  Initialize();

  ASSERT_TRUE(embedded_test_server()->Start());

  content::WebContents* left_tab =
      OpenTestPageAndGetUserMediaInNewTab(kMainWebrtcTestHtmlPage);
  content::WebContents* right_tab =
      OpenTestPageAndGetUserMediaInNewTab(kMainWebrtcTestHtmlPage);

  SetupPeerconnectionWithLocalStream(left_tab);
  SetupPeerconnectionWithLocalStream(right_tab);

  SetDefaultVideoCodec(left_tab, "VP8");
  SetDefaultVideoCodec(right_tab, "VP8");
  NegotiateCall(left_tab, right_tab);

  StartDetectingVideo(left_tab, "remote-view");
  StartDetectingVideo(right_tab, "remote-view");

  // Start the event log.
  const int seconds = 0;
  base::ListValue start_params;
  AppendTabIdAndUrl(&start_params, left_tab);
  start_params.AppendInteger(seconds);
  scoped_refptr<WebrtcLoggingPrivateStartWebRtcEventLoggingFunction>
      start_function(CreateExtensionFunction<
                     WebrtcLoggingPrivateStartWebRtcEventLoggingFunction>());
  std::unique_ptr<base::Value> start_result(
      utils::RunFunctionAndReturnSingleResult(
          start_function.get(), ParamsToString(start_params), browser()));
  ASSERT_TRUE(start_result.get());

  // Get the file name.
  std::unique_ptr<extensions::api::webrtc_logging_private::RecordingInfo>
      recordings_info_start(
          extensions::api::webrtc_logging_private::RecordingInfo::FromValue(
              *start_result));
  ASSERT_TRUE(recordings_info_start.get());
  base::FilePath file_name_start(
      base::FilePath::FromUTF8Unsafe(recordings_info_start->prefix_path));

#if !defined(OS_MACOSX)
  // Video is choppy on Mac OS X. http://crbug.com/443542.
  WaitForVideoToPlay(left_tab);
  WaitForVideoToPlay(right_tab);
#else
  // This is a hack, but that shouldn't be a problem, because we're just about
  // remove this code anyway.
  // TODO(eladalon): Remove this. http://crbug.com/775415
  base::WaitableEvent wait(base::WaitableEvent::ResetPolicy::MANUAL,
                           base::WaitableEvent::InitialState::NOT_SIGNALED);
  wait.TimedWait(base::TimeDelta::FromSeconds(1));
#endif

  // Stop the event log.
  base::ListValue stop_params;
  AppendTabIdAndUrl(&stop_params, left_tab);
  scoped_refptr<WebrtcLoggingPrivateStopWebRtcEventLoggingFunction>
      stop_function(CreateExtensionFunction<
                    WebrtcLoggingPrivateStopWebRtcEventLoggingFunction>());
  std::unique_ptr<base::Value> stop_result(
      utils::RunFunctionAndReturnSingleResult(
          stop_function.get(), ParamsToString(stop_params), browser()));

  // Get the file name.
  std::unique_ptr<extensions::api::webrtc_logging_private::RecordingInfo>
      recordings_info_stop(
          extensions::api::webrtc_logging_private::RecordingInfo::FromValue(
              *stop_result));
  ASSERT_TRUE(recordings_info_stop.get());
  base::FilePath file_name_stop(
      base::FilePath::FromUTF8Unsafe(recordings_info_stop->prefix_path));

  HangUp(left_tab);
  HangUp(right_tab);

  EXPECT_EQ(file_name_start, file_name_stop);

  // Check that the file exists and is non-empty.
  content::RenderProcessHost* render_process_host =
      left_tab->GetMainFrame()->GetProcess();
  ASSERT_NE(render_process_host, nullptr);
  int render_process_id = render_process_host->GetID();
  base::FilePath full_file_name =
      GetExpectedEventLogFileName(file_name_stop, render_process_id);
  int64_t file_size = 0;
  scoped_refptr<FileWaiter> waiter = new FileWaiter(full_file_name);

  ASSERT_TRUE(waiter->Start()) << "ERROR watching for "
                               << full_file_name.value();
  ASSERT_TRUE(waiter->WaitForFile());
  base::ScopedAllowBlockingForTesting allow_blocking;
  ASSERT_TRUE(base::PathExists(full_file_name));
  EXPECT_TRUE(base::GetFileSize(full_file_name, &file_size));
  EXPECT_GT(file_size, 0);

  // Clean up.
  base::DeleteFile(full_file_name, false);
}

IN_PROC_BROWSER_TEST_F(WebrtcEventLogApiTest,
                       TestStartTimedWebRtcEventLogging) {
  Initialize();

  base::ScopedAllowBlockingForTesting allow_blocking;
  ASSERT_TRUE(embedded_test_server()->Start());

  content::WebContents* left_tab =
      OpenTestPageAndGetUserMediaInNewTab(kMainWebrtcTestHtmlPage);
  content::WebContents* right_tab =
      OpenTestPageAndGetUserMediaInNewTab(kMainWebrtcTestHtmlPage);

  SetupPeerconnectionWithLocalStream(left_tab);
  SetupPeerconnectionWithLocalStream(right_tab);

  SetDefaultVideoCodec(left_tab, "VP8");
  SetDefaultVideoCodec(right_tab, "VP8");
  NegotiateCall(left_tab, right_tab);

  StartDetectingVideo(left_tab, "remote-view");
  StartDetectingVideo(right_tab, "remote-view");

  // Start the event log. RunFunctionAndReturnSingleResult will block until a
  // result is available, which happens when the logging stops after 1 second.
  const int seconds = 1;
  base::ListValue start_params;
  AppendTabIdAndUrl(&start_params, left_tab);
  start_params.AppendInteger(seconds);
  scoped_refptr<WebrtcLoggingPrivateStartWebRtcEventLoggingFunction>
      start_function(CreateExtensionFunction<
                     WebrtcLoggingPrivateStartWebRtcEventLoggingFunction>());
  std::unique_ptr<base::Value> start_result(
      utils::RunFunctionAndReturnSingleResult(
          start_function.get(), ParamsToString(start_params), browser()));
  ASSERT_TRUE(start_result.get());

  // Get the file name.
  std::unique_ptr<extensions::api::webrtc_logging_private::RecordingInfo>
      recordings_info_start(
          extensions::api::webrtc_logging_private::RecordingInfo::FromValue(
              *start_result));
  ASSERT_TRUE(recordings_info_start.get());
  base::FilePath file_name_start(
      base::FilePath::FromUTF8Unsafe(recordings_info_start->prefix_path));

#if !defined(OS_MACOSX)
  // Video is choppy on Mac OS X. http://crbug.com/443542.
  WaitForVideoToPlay(left_tab);
  WaitForVideoToPlay(right_tab);
#endif

  HangUp(left_tab);
  HangUp(right_tab);

  // The log has stopped automatically. Check that the file exists and is
  // non-empty.
  content::RenderProcessHost* render_process_host =
      left_tab->GetMainFrame()->GetProcess();
  ASSERT_NE(render_process_host, nullptr);
  int render_process_id = render_process_host->GetID();
  base::FilePath full_file_name =
      GetExpectedEventLogFileName(file_name_start, render_process_id);
  int64_t file_size = 0;

  scoped_refptr<FileWaiter> waiter = new FileWaiter(full_file_name);

  ASSERT_TRUE(waiter->Start()) << "ERROR watching for "
                               << full_file_name.value();
  ASSERT_TRUE(waiter->WaitForFile());
  ASSERT_TRUE(base::PathExists(full_file_name));
  EXPECT_TRUE(base::GetFileSize(full_file_name, &file_size));
  EXPECT_GT(file_size, 0);

  // Clean up.
  base::DeleteFile(full_file_name, false);
}
