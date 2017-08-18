// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/crash/content/app/export_thunks.h"

#include "base/strings/utf_string_conversions.h"
#include "components/crash/content/app/crashpad.h"
#include "third_party/crashpad/crashpad/client/crashpad_client.h"

void RequestSingleCrashUploadThunk(const std::string& local_id) {
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

int CrashForException(EXCEPTION_POINTERS* info) {
  crash_reporter::GetCrashpadClient().DumpAndCrash(info);
  return EXCEPTION_CONTINUE_SEARCH;
}

// This function is used in chrome_metrics_services_manager_client.cc to trigger
// changes to the upload-enabled state. This is done when the metrics services
// are initialized, and when the user changes their consent for uploads. See
// crash_reporter::SetUploadConsent for effects. The given consent value should
// be consistent with
// crash_reporter::GetCrashReporterClient()->GetCollectStatsConsent(), but it's
// not enforced to avoid blocking startup code on synchronizing them.
void SetUploadConsentImpl(bool consent) {
  crash_reporter::SetUploadConsent(consent);
}

// NOTE: This function is used by SyzyASAN to annotate crash reports. If you
// change the name or signature of this function you will break SyzyASAN
// instrumented releases of Chrome. Please contact syzygy-team@chromium.org
// before doing so! See also http://crbug.com/567781.
void SetCrashKeyValueImpl(const wchar_t* key, const wchar_t* value) {
  crash_reporter::SetCrashKeyValue(base::UTF16ToUTF8(key),
                                   base::UTF16ToUTF8(value));
}

void ClearCrashKeyValueImpl(const wchar_t* key) {
  crash_reporter::ClearCrashKey(base::UTF16ToUTF8(key));
}

void SetCrashKeyValueImplEx(const char* key, const char* value) {
  crash_reporter::SetCrashKeyValue(key, value);
}

void ClearCrashKeyValueImplEx(const char* key) {
  crash_reporter::ClearCrashKey(key);
}
