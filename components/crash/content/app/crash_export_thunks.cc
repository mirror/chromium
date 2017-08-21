// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/crash/content/app/crash_export_thunks.h"

#include "components/crash/content/app/crashpad.h"

void RequestSingleCrashUploadThunk(const char* local_id) {
  crash_reporter::RequestSingleCrashUploadImpl(local_id);
}

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
