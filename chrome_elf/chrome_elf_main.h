// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_ELF_CHROME_ELF_MAIN_H_
#define CHROME_ELF_CHROME_ELF_MAIN_H_

#include <windows.h>

#include <string>

namespace crash_reporter {
struct Report;
}

// These functions are the cross-module import interface to chrome_elf.dll.
// It is used chrome.exe, chrome.dll and other clients of chrome_elf.
// In tests, these functions are stubbed by implementations in
// chrome_elf_test_stubs.cc.
extern "C" {

void DumpProcessWithoutCrash();
void GetUserDataDirectoryThunk(wchar_t* user_data_dir,
                               size_t user_data_dir_length,
                               wchar_t* invalid_user_data_dir,
                               size_t invalid_user_data_dir_length);
// This function is a temporary workaround for https://crbug.com/655788. We
// need to come up with a better way to initialize crash reporting that can
// happen inside DllMain().
void SignalInitializeCrashReporting();
void SignalChromeElf();

int CrashForException(EXCEPTION_POINTERS* info);

HANDLE __cdecl InjectDumpForHungInput(HANDLE process,
                                      void* serialized_crash_keys);

HANDLE __cdecl InjectDumpForHungInputNoCrashKeys(HANDLE process, int reason);

void __cdecl RegisterNonABICompliantCodeRange(void* start,
                                              size_t size_in_bytes);

void __cdecl UnregisterNonABICompliantCodeRange(void* start);

void GetCrashReportsImpl(const crash_reporter::Report** reports,
                         size_t* report_count);

void SetMetricsClientId(const char* client_id);

void SetUploadConsentImpl(bool consent);

// NOTE: This function is used by SyzyASAN to annotate crash reports. If you
// change the name or signature of this function you will break SyzyASAN
// instrumented releases of Chrome. Please contact syzygy-team@chromium.org
// before doing so! See also http://crbug.com/567781.
void SetCrashKeyValueImpl(const wchar_t* key, const wchar_t* value);

void ClearCrashKeyValueImpl(const wchar_t* key);

// This helper is invoked by code in chrome.dll to request a single crash report
// upload. See CrashUploadListCrashpad.
// TODO(siggi): std::string across module boundaries seems ... impertinent.
void RequestSingleCrashUploadImpl(const std::string& local_id);

}  // extern "C"

#endif  // CHROME_ELF_CHROME_ELF_MAIN_H_
