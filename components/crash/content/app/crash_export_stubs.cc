// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/crash/content/app/crash_export_thunks.h"
#include "components/crash/content/app/crashpad.h"

void RequestSingleCrashUploadThunk(const char* local_id) {
}

size_t GetCrashReportsImpl(crash_reporter::Report* reports,
                           size_t reports_size) {
  return 0;
}

int CrashForException(EXCEPTION_POINTERS* info) {
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
}

// NOTE: This function is used by SyzyASAN to annotate crash reports. If you
// change the name or signature of this function you will break SyzyASAN
// instrumented releases of Chrome. Please contact syzygy-team@chromium.org
// before doing so! See also http://crbug.com/567781.
void SetCrashKeyValueImpl(const wchar_t* key, const wchar_t* value) {
}

void ClearCrashKeyValueImpl(const wchar_t* key) {
}

void SetCrashKeyValueImplEx(const char* key, const char* value) {
}

void ClearCrashKeyValueImplEx(const char* key) {
}
