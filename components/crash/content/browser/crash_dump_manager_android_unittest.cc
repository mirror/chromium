// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/crash/content/browser/crash_dump_manager_android.h"

#include <utility>

#include "base/at_exit.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/files/file.h"
#include "base/files/scoped_file.h"
#include "base/files/scoped_temp_dir.h"
#include "base/macros.h"
#include "base/optional.h"
#include "base/run_loop.h"
#include "base/task_scheduler/post_task.h"
#include "base/test/histogram_tester.h"
#include "base/threading/thread_restrictions.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/test/test_renderer_host.h"

namespace breakpad {

class CrashDumpManagerTest : public content::RenderViewHostTestHarness {
 public:
  CrashDumpManagerTest() {}
  ~CrashDumpManagerTest() override {}

  void SetUp() override {
    // Initialize the manager.
    CrashDumpManager::GetInstance();
    content::RenderViewHostTestHarness::SetUp();
  }

  // TODO(csharrison): This test harness is not robust enough. Namely, it does
  // not support actually processing a non-empty minidump, due to JNI calls.
  static void CreateAndProcessEmptyMinidump(
      int process_host_id,
      content::ProcessType process_type,
      base::TerminationStatus status,
      base::android::ApplicationState app_state) {
    base::ScopedFD fd =
        CrashDumpManager::GetInstance()->CreateMinidumpFileForChild(
            process_host_id);
    EXPECT_TRUE(fd.is_valid());
    base::ScopedTempDir dump_dir;
    EXPECT_TRUE(dump_dir.CreateUniqueTempDir());
    CrashDumpManager::GetInstance()->ProcessMinidumpFileFromChild(
        dump_dir.GetPath(), process_host_id, process_type, status, app_state);
  }

 private:
  base::AtExitManager at_exit_manager_;
  DISALLOW_COPY_AND_ASSIGN(CrashDumpManagerTest);
};

class MinidumpObserver : public CrashDumpManager::Observer {
 public:
  MinidumpObserver() {}
  ~MinidumpObserver() {}

  // CrashDumpManager::Observer:
  void OnMinidumpProcessed(
      const CrashDumpManager::MinidumpDetails& details) override {
    last_details_ = details;
    if (!wait_closure_.is_null())
      std::move(wait_closure_).Run();
  }

  const CrashDumpManager::MinidumpDetails& last_details() const {
    return last_details_.value();
  }

  void WaitForProcessed() {
    base::RunLoop run_loop;
    wait_closure_ = run_loop.QuitClosure();
    run_loop.Run();
  }

 private:
  base::OnceClosure wait_closure_;
  base::Optional<CrashDumpManager::MinidumpDetails> last_details_;
  DISALLOW_COPY_AND_ASSIGN(MinidumpObserver);
};

TEST_F(CrashDumpManagerTest, SimpleOOM) {
  base::HistogramTester histogram_tester;
  CrashDumpManager* manager = CrashDumpManager::GetInstance();

  MinidumpObserver minidump_observer;
  manager->AddObserver(&minidump_observer);

  int process_host_id = main_rfh()->GetProcess()->GetID();
  base::PostTaskWithTraits(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::Bind(&CrashDumpManagerTest::CreateAndProcessEmptyMinidump,
                 process_host_id, content::PROCESS_TYPE_RENDERER,
                 base::TERMINATION_STATUS_OOM_PROTECTED,
                 base::android::APPLICATION_STATE_HAS_RUNNING_ACTIVITIES));
  minidump_observer.WaitForProcessed();

  const CrashDumpManager::MinidumpDetails& details =
      minidump_observer.last_details();
  EXPECT_EQ(process_host_id, details.process_host_id);
  EXPECT_EQ(content::PROCESS_TYPE_RENDERER, details.process_type);
  EXPECT_EQ(base::TERMINATION_STATUS_OOM_PROTECTED, details.termination_status);
  EXPECT_EQ(base::android::APPLICATION_STATE_HAS_RUNNING_ACTIVITIES,
            details.app_state);
  EXPECT_EQ(0, details.file_size);

  histogram_tester.ExpectUniqueSample(
      "Tab.RendererDetailedExitStatus",
      CrashDumpManager::EMPTY_MINIDUMP_WHILE_RUNNING, 1);
}

}  // namespace breakpad
