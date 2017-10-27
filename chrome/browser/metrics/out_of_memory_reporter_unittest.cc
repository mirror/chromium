// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/out_of_memory_reporter.h"

#include <memory>
#include <utility>

#include "base/at_exit.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/files/scoped_file.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/optional.h"
#include "base/process/kill.h"
#include "base/run_loop.h"
#include "base/task_scheduler/post_task.h"
#include "build/build_config.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/mock_render_process_host.h"
#include "content/public/test/navigation_simulator.h"
#include "content/public/test/test_renderer_host.h"
#include "net/base/net_errors.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

#if defined(OS_ANDROID)
#include "components/crash/content/browser/crash_dump_manager_android.h"
#endif

class OutOfMemoryReporterTest : public ChromeRenderViewHostTestHarness,
                                public OutOfMemoryReporter::Observer {
 public:
  OutOfMemoryReporterTest() {}
  ~OutOfMemoryReporterTest() override {}

  // ChromeRenderViewHostTestHarness:
  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
    OutOfMemoryReporter::CreateForWebContents(web_contents());
    OutOfMemoryReporter::FromWebContents(web_contents())->AddObserver(this);
  }

  // OutOfMemoryReporter::Observer:
  void OnForegroundOOMDetected(const GURL& url,
                               ukm::SourceId source_id) override {
    last_oom_url_ = url;
    if (!oom_closure_.is_null())
      std::move(oom_closure_).Run();
  }

 protected:
  base::ShadowingAtExitManager at_exit_;

  base::Optional<GURL> last_oom_url_;
  base::OnceClosure oom_closure_;

 private:
  DISALLOW_COPY_AND_ASSIGN(OutOfMemoryReporterTest);
};

TEST_F(OutOfMemoryReporterTest, SimpleOOM) {
  const GURL url("https://example.test/");
  NavigateAndCommit(url);

  process()->SimulateRenderProcessExit(base::TERMINATION_STATUS_OOM, 0);

  // Should get the notification synchronously.
  EXPECT_EQ(url, last_oom_url_.value());
}

TEST_F(OutOfMemoryReporterTest, NormalCrash_NoOOM) {
  const GURL url("https://example.test/");
  NavigateAndCommit(url);

  process()->SimulateRenderProcessExit(
      base::TERMINATION_STATUS_ABNORMAL_TERMINATION, 0);

  // Would get the notification synchronously.
  EXPECT_FALSE(last_oom_url_.has_value());
}

TEST_F(OutOfMemoryReporterTest, OOMOnPreviousPage) {
  const GURL url1("https://example.test1/");
  const GURL url2("https://example.test2/");
  const GURL url3("https://example.test2/");
  NavigateAndCommit(url1);
  NavigateAndCommit(url2);

  // Should not commit.
  content::NavigationSimulator::NavigateAndFailFromBrowser(web_contents(), url3,
                                                           net::ERR_ABORTED);
  process()->SimulateRenderProcessExit(base::TERMINATION_STATUS_OOM, 0);

  // Would get the notification synchronously.
  EXPECT_EQ(url2, last_oom_url_.value());
  last_oom_url_.reset();

  NavigateAndCommit(url1);

  // Should navigate to an error page.
  content::NavigationSimulator::NavigateAndFailFromBrowser(
      web_contents(), url3, net::ERR_CONNECTION_RESET);
  // Don't report OOMs on error pages.
  process()->SimulateRenderProcessExit(base::TERMINATION_STATUS_OOM, 0);
  EXPECT_FALSE(last_oom_url_.has_value());
}

#if defined(OS_ANDROID)
TEST_F(OutOfMemoryReporterTest, ReportOOMViaMinidump) {
  const GURL url("https://example.test/");
  NavigateAndCommit(url);

  // Simulate OOM by crashing normally, and processing an empty minidump.
  process()->SimulateRenderProcessExit(base::TERMINATION_STATUS_OOM_PROTECTED,
                                       0);

  EXPECT_FALSE(last_oom_url_.has_value());

  auto create_and_process_minidump =
      [](int process_host_id, content::ProcessType process_type,
         base::TerminationStatus status,
         base::android::ApplicationState app_state) {
        auto* manager = breakpad::CrashDumpManager::GetInstance();
        base::ScopedFD fd =
            manager->CreateMinidumpFileForChild(process_host_id);
        EXPECT_TRUE(fd.is_valid());
        base::ScopedTempDir dump_dir;
        EXPECT_TRUE(dump_dir.CreateUniqueTempDir());
        manager->ProcessMinidumpFileFromChild(dump_dir.GetPath(),
                                              process_host_id, process_type,
                                              status, app_state);
      };
  base::PostTaskWithTraits(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::Bind(create_and_process_minidump, process()->GetID(),
                 content::PROCESS_TYPE_RENDERER,
                 base::TERMINATION_STATUS_OOM_PROTECTED,
                 base::android::APPLICATION_STATE_HAS_RUNNING_ACTIVITIES));

  base::RunLoop run_loop;
  oom_closure_ = run_loop.QuitClosure();
  run_loop.Run();
  EXPECT_EQ(url, last_oom_url_.value());
}
#endif  // defined(OS_ANDROID)
