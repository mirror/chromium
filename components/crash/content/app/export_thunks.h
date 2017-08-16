// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRASH_CONTENT_APP_EXPORT_THUNKS_H_
#define COMPONENTS_CRASH_CONTENT_APP_EXPORT_THUNKS_H_

#include "components/crash/content/app/crashpad.h"

extern "C" {

// TODO(siggi): Rename these functions to something descriptive and unique,
//   once all the thunks have been converted to import binding.

// This function may be invoked across module boundaries to request a single
// crash report upload. See CrashUploadListCrashpad.
void RequestSingleCrashUploadImpl(const std::string& local_id);

// This helper is invoked by code in chrome.dll to retrieve the crash reports.
// See CrashUploadListCrashpad. Note that we do not pass a std::vector here,
// because we do not want to allocate/free in different modules. The returned
// pointer is read-only.
//
// NOTE: Since the returned pointer references read-only memory that will be
// cleaned up when this DLL unloads, be careful not to reference the memory
// beyond that point (e.g. during tests).
void GetCrashReportsImpl(const crash_reporter::Report** reports,
                         size_t* report_count);

}  // extern "C"

#endif  // COMPONENTS_CRASH_CONTENT_APP_EXPORT_THUNKS_H_
