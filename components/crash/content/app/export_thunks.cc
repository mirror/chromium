// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/crash/content/app/export_thunks.h"

#include "components/crash/content/app/crashpad.h"
#include "third_party/crashpad/crashpad/client/crashpad_client.h"

void RequestSingleCrashUploadThunk(const std::string& local_id) {
  crash_reporter::RequestSingleCrashUploadImpl(local_id);
}

// This helper is invoked by code in chrome.dll to retrieve the crash reports.
// See CrashUploadListCrashpad. Note that we do not pass a std::vector here,
// because we do not want to allocate/free in different modules. The returned
// pointer is read-only.
//
// NOTE: Since the returned pointer references read-only memory that will be
// cleaned up when this DLL unloads, be careful not to reference the memory
// beyond that point (e.g. during tests).
void GetCrashReportsImpl(const crash_reporter::Report** reports,
                         size_t* report_count) {
  // Since this could be called across module boundaries, this vector provides
  // storage for the result on the far side of the boundary.
  static std::vector<crash_reporter::Report> crash_reports;

  // The crash_reporter::GetReports function thunks here, which delegates to
  // the actual implementation.
  crash_reporter::GetReportsImpl(&crash_reports);
  *reports = crash_reports.data();
  *report_count = crash_reports.size();
}

// Crashes the process after generating a dump for the provided exception. Note
// that the crash reporter should be initialized before calling this function
// for it to do anything.
// NOTE: This function is used by SyzyASAN to invoke a crash. If you change the
// the name or signature of this function you will break SyzyASAN instrumented
// releases of Chrome. Please contact syzygy-team@chromium.org before doing so!
int CrashForException(EXCEPTION_POINTERS* info) {
  crash_reporter::GetCrashpadClient().DumpAndCrash(info);
  return EXCEPTION_CONTINUE_SEARCH;
}
