// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/out_of_memory_reporter.h"

#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"

OutOfMemoryReporter::OutOfMemoryReporter(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents)
#if defined(OS_ANDROID)
      ,
      scoped_observer_(this) {
  auto* crash_manager = breakpad::CrashDumpManager::GetInstance();
  DCHECK(crash_manager);
  scoped_observer_.Add(crash_manager);
#else
  {
#endif
}

OutOfMemoryReporter::~OutOfMemoryReporter() {}

void OutOfMemoryReporter::OnForegroundOOMDetected(ukm::SourceId source_id) {}

void OutOfMemoryReporter::DidFinishNavigation(
    content::NavigationHandle* handle) {
  // Only care about navigations that commit to another document.
  if (!handle->HasCommitted() || handle->IsSameDocument())
    return;
  last_committed_source_id_.reset();
  crashed_render_process_id_ = content::ChildProcessHost::kInvalidUniqueID;
  if (handle->IsErrorPage())
    return;
  last_committed_source_id_ = ukm::ConvertToSourceId(
      handle->GetNavigationId(), ukm::SourceIdType::NAVIGATION_ID);
}

void OutOfMemoryReporter::RenderProcessGone(base::TerminationStatus status) {
  if (!last_committed_source_id_.has_value())
    return;
  if (!web_contents()->IsVisible())
    return;

  crashed_render_process_id_ =
      web_contents()->GetMainFrame()->GetProcess()->GetID();

#if defined(OS_CHROMEOS)
  if (status == base::TERMINATION_STATUS_PROCESS_WAS_KILLED_BY_OOM) {
    OnForegroundOOMDetected(*last_committed_source_id_);
  }
#else
  if (status == base::TERMINATION_STATUS_OOM) {
    OnForegroundOOMDetected(*last_committed_source_id_);
  }
#endif
}

#if defined(OS_ANDROID)
void OutOfMemoryReporter::OnMinidumpProcessed(
    const breakpad::CrashDumpManager::MinidumpDetails& details) {
  if (!last_committed_source_id_.has_value())
    return;
  if (details.process_type == content::PROCESS_TYPE_RENDERER &&
      details.termination_status == base::TERMINATION_STATUS_OOM_PROTECTED &&
      details.file_size == 0 &&
      details.app_state ==
          base::android::APPLICATION_STATE_HAS_RUNNING_ACTIVITIES &&
      details.process_host_id == crashed_render_process_id_) {
    OnForegroundOOMDetected(*last_committed_source_id_);
  }
}
#endif  // defined(OS_ANDROID)
