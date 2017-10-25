// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_METRICS_OUT_OF_MEMORY_REPORTER_H_
#define CHROME_BROWSER_METRICS_OUT_OF_MEMORY_REPORTER_H_

#include "base/macros.h"
#include "base/scoped_observer.h"
#include "build/build_config.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/common/child_process_host.h"
#include "services/metrics/public/cpp/ukm_source_id.h"
#include "url/gurl.h"

#if defined(OS_ANDROID)
#include "components/crash/content/browser/crash_dump_manager_android.h"
#endif

class OutOfMemoryReporter : public content::WebContentsObserver
#if defined(OS_ANDROID)
    ,
                            public breakpad::CrashDumpManager::Observer
#endif
{
 public:
  OutOfMemoryReporter(content::WebContents* web_contents);
  ~OutOfMemoryReporter() override;

  void OnForegroundOOMDetected(ukm::SourceId source_id);

 private:
  // content::WebContentsObserver:
  void DidFinishNavigation(content::NavigationHandle* handle) override;
  void RenderProcessGone(base::TerminationStatus termination_status) override;

// breakpad::CrashDumpManager::Observer:
#if defined(OS_ANDROID)
  void OnMinidumpProcessed(
      const breakpad::CrashDumpManager::MinidumpDetails& details) override;

  ScopedObserver<breakpad::CrashDumpManager,
                 breakpad::CrashDumpManager::Observer>
      scoped_observer_;
#endif  // defined(OS_ANDROID)

  base::Optional<ukm::SourceId> last_committed_source_id_;
  int crashed_render_process_id_ = content::ChildProcessHost::kInvalidUniqueID;

  DISALLOW_COPY_AND_ASSIGN(OutOfMemoryReporter);
};

#endif  // CHROME_BROWSER_METRICS_OUT_OF_MEMORY_REPORTER_H_
