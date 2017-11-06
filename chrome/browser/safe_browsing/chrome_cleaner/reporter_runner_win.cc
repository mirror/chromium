// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/safe_browsing/chrome_cleaner/reporter_runner_win.h"

#include <stdint.h>

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "base/debug/leak_annotations.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/memory/weak_ptr.h"
#include "base/metrics/field_trial.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/sparse_histogram.h"
#include "base/process/launch.h"
#include "base/process/process.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_runner_util.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "base/version.h"
#include "base/win/registry.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/metrics/chrome_metrics_service_accessor.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/safe_browsing/chrome_cleaner/chrome_cleaner_controller_win.h"
#include "chrome/browser/safe_browsing/chrome_cleaner/chrome_cleaner_dialog_controller_impl_win.h"
#include "chrome/browser/safe_browsing/chrome_cleaner/chrome_cleaner_fetcher_win.h"
#include "chrome/browser/safe_browsing/chrome_cleaner/srt_client_info_win.h"
#include "chrome/browser/safe_browsing/chrome_cleaner/srt_field_trial_win.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/common/pref_names.h"
#include "components/chrome_cleaner/public/constants/constants.h"
#include "components/component_updater/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/version_info/version_info.h"
#include "content/public/browser/browser_thread.h"
#include "net/http/http_status_code.h"

using content::BrowserThread;

namespace safe_browsing {

namespace {

// Used to send UMA information about missing start and end time registry
// values for the reporter. Replicated in the histograms.xml file, so the order
// MUST NOT CHANGE.
enum SwReporterRunningTimeRegistryError {
  REPORTER_RUNNING_TIME_ERROR_NO_ERROR = 0,
  REPORTER_RUNNING_TIME_ERROR_REGISTRY_KEY_INVALID = 1,
  REPORTER_RUNNING_TIME_ERROR_MISSING_START_TIME = 2,
  REPORTER_RUNNING_TIME_ERROR_MISSING_END_TIME = 3,
  REPORTER_RUNNING_TIME_ERROR_MISSING_BOTH_TIMES = 4,
  REPORTER_RUNNING_TIME_ERROR_MAX,
};

// Used to send UMA information about the progress of the SwReporter launch and
// prompt sequence. Replicated in the histograms.xml file, so the order MUST
// NOT CHANGE.
enum SwReporterUmaValue {
  DEPRECATED_SW_REPORTER_EXPLICIT_REQUEST = 0,
  DEPRECATED_SW_REPORTER_STARTUP_RETRY = 1,
  DEPRECATED_SW_REPORTER_RETRIED_TOO_MANY_TIMES = 2,
  SW_REPORTER_START_EXECUTION = 3,
  SW_REPORTER_FAILED_TO_START = 4,
  DEPRECATED_SW_REPORTER_REGISTRY_EXIT_CODE = 5,
  DEPRECATED_SW_REPORTER_RESET_RETRIES = 6,
  DEPRECATED_SW_REPORTER_DOWNLOAD_START = 7,
  SW_REPORTER_NO_BROWSER = 8,
  DEPRECATED_SW_REPORTER_NO_LOCAL_STATE = 9,
  SW_REPORTER_NO_PROMPT_NEEDED = 10,
  SW_REPORTER_NO_PROMPT_FIELD_TRIAL = 11,
  SW_REPORTER_ALREADY_PROMPTED = 12,
  DEPRECATED_SW_REPORTER_RAN_DAILY = 13,
  DEPRECATED_SW_REPORTER_ADDED_TO_MENU = 14,

  SW_REPORTER_MAX,
};

// Used to send UMA information showing whether uploading of Software Reporter
// logs is enabled, or the reason why not.
// Replicated in the histograms.xml file, so the order MUST NOT CHANGE.
enum SwReporterLogsUploadsEnabled {
  REPORTER_LOGS_UPLOADS_SBER_ENABLED = 0,
  REPORTER_LOGS_UPLOADS_SBER_DISABLED = 1,
  REPORTER_LOGS_UPLOADS_RECENTLY_SENT_LOGS = 2,
  REPORTER_LOGS_UPLOADS_DISABLED_BY_USER = 3,
  REPORTER_LOGS_UPLOADS_ENABLED_BY_USER = 4,
  REPORTER_LOGS_UPLOADS_MAX,
};

// Used to send UMA information about missing logs upload result in the registry
// for the reporter. Replicated in the histograms.xml file, so the order
// MUST NOT CHANGE.
enum SwReporterLogsUploadResultRegistryError {
  REPORTER_LOGS_UPLOAD_RESULT_ERROR_NO_ERROR = 0,
  REPORTER_LOGS_UPLOAD_RESULT_ERROR_REGISTRY_KEY_INVALID = 1,
  REPORTER_LOGS_UPLOAD_RESULT_ERROR_VALUE_NOT_FOUND = 2,
  REPORTER_LOGS_UPLOAD_RESULT_ERROR_VALUE_OUT_OF_BOUNDS = 3,
  REPORTER_LOGS_UPLOAD_RESULT_ERROR_MAX,
};

const char kRunningTimeErrorMetricName[] =
    "SoftwareReporter.RunningTimeRegistryError";

SwReporterTestingDelegate* g_testing_delegate_ = nullptr;

const char kFoundUwsMetricName[] = "SoftwareReporter.FoundUwS";
const char kFoundUwsReadErrorMetricName[] =
    "SoftwareReporter.FoundUwSReadError";
const char kScanTimesMetricName[] = "SoftwareReporter.UwSScanTimes";
const char kMemoryUsedMetricName[] = "SoftwareReporter.MemoryUsed";
const char kStepMetricName[] = "SoftwareReporter.Step";
const char kLogsUploadEnabledMetricName[] =
    "SoftwareReporter.LogsUploadEnabled";
const char kLogsUploadResultMetricName[] = "SoftwareReporter.LogsUploadResult";
const char kLogsUploadResultRegistryErrorMetricName[] =
    "SoftwareReporter.LogsUploadResultRegistryError";
const char kExitCodeMetricName[] = "SoftwareReporter.ExitCodeFromRegistry";
const char kEngineErrorCodeMetricName[] = "SoftwareReporter.EngineErrorCode";

// The max value for histogram SoftwareReporter.LogsUploadResult, which is used
// to send UMA information about the result of Software Reporter's attempt to
// upload logs, when logs are enabled. This value must be consistent with the
// SoftwareReporterLogsUploadResult enum defined in the histograms.xml file.
const int kSwReporterLogsUploadResultMax = 30;

// Reports metrics about the software reporter via UMA (and sometimes Rappor).
class UMAHistogramReporter {
 public:
  UMAHistogramReporter() : UMAHistogramReporter(std::string()) {}

  explicit UMAHistogramReporter(const std::string& suffix)
      : suffix_(suffix),
        registry_key_(suffix.empty()
                          ? chrome_cleaner::kSoftwareRemovalToolRegistryKey
                          : base::StringPrintf(
                                L"%ls\\%ls",
                                chrome_cleaner::kSoftwareRemovalToolRegistryKey,
                                base::UTF8ToUTF16(suffix).c_str())) {}

  // Reports the software reporter tool's version via UMA.
  void ReportVersion(const base::Version& version) const {
    DCHECK(!version.components().empty());
    // The minor version is the 2nd last component of the version,
    // or just the first component if there is only 1.
    uint32_t minor_version = 0;
    if (version.components().size() > 1)
      minor_version = version.components()[version.components().size() - 2];
    else
      minor_version = version.components()[0];
    RecordSparseHistogram("SoftwareReporter.MinorVersion", minor_version);

    // The major version for X.Y.Z is X*256^3+Y*256+Z. If there are additional
    // components, only the first three count, and if there are less than 3, the
    // missing values are just replaced by zero. So 1 is equivalent 1.0.0.
    DCHECK_LT(version.components()[0], 0x100U);
    uint32_t major_version = 0x1000000 * version.components()[0];
    if (version.components().size() >= 2) {
      DCHECK_LT(version.components()[1], 0x10000U);
      major_version += 0x100 * version.components()[1];
    }
    if (version.components().size() >= 3) {
      DCHECK_LT(version.components()[2], 0x100U);
      major_version += version.components()[2];
    }
    RecordSparseHistogram("SoftwareReporter.MajorVersion", major_version);
  }

  void ReportExitCode(int exit_code) const {
    RecordSparseHistogram("SoftwareReporter.ExitCode", exit_code);

    // Also report the exit code that the reporter writes to the registry.
    base::win::RegKey reporter_key;
    DWORD exit_code_in_registry;
    if (reporter_key.Open(HKEY_CURRENT_USER, registry_key_.c_str(),
                          KEY_QUERY_VALUE | KEY_SET_VALUE) != ERROR_SUCCESS ||
        reporter_key.ReadValueDW(chrome_cleaner::kExitCodeValueName,
                                 &exit_code_in_registry) != ERROR_SUCCESS) {
      return;
    }

    RecordSparseHistogram(kExitCodeMetricName, exit_code_in_registry);
    reporter_key.DeleteValue(chrome_cleaner::kExitCodeValueName);
  }

  void ReportEngineErrorCode() const {
    base::win::RegKey reporter_key;
    DWORD engine_error_code;
    if (reporter_key.Open(HKEY_CURRENT_USER, registry_key_.c_str(),
                          KEY_QUERY_VALUE | KEY_SET_VALUE) != ERROR_SUCCESS ||
        reporter_key.ReadValueDW(chrome_cleaner::kEngineErrorCodeValueName,
                                 &engine_error_code) != ERROR_SUCCESS) {
      return;
    }

    RecordSparseHistogram(kEngineErrorCodeMetricName, engine_error_code);
    reporter_key.DeleteValue(chrome_cleaner::kEngineErrorCodeValueName);
  }

  // Reports UwS found by the software reporter tool via UMA and RAPPOR.
  void ReportFoundUwS() const {
    base::win::RegKey reporter_key;
    std::vector<base::string16> found_uws_strings;
    if (reporter_key.Open(HKEY_CURRENT_USER, registry_key_.c_str(),
                          KEY_QUERY_VALUE | KEY_SET_VALUE) != ERROR_SUCCESS ||
        reporter_key.ReadValues(chrome_cleaner::kFoundUwsValueName,
                                &found_uws_strings) != ERROR_SUCCESS) {
      return;
    }

    bool parse_error = false;
    for (const base::string16& uws_string : found_uws_strings) {
      // All UwS ids are expected to be integers.
      uint32_t uws_id = 0;
      if (base::StringToUint(uws_string, &uws_id)) {
        RecordSparseHistogram(kFoundUwsMetricName, uws_id);
      } else {
        parse_error = true;
      }
    }

    // Clean up the old value.
    reporter_key.DeleteValue(chrome_cleaner::kFoundUwsValueName);
    RecordBooleanHistogram(kFoundUwsReadErrorMetricName, parse_error);
  }

  // Reports to UMA the memory usage of the software reporter tool as reported
  // by the tool itself in the Windows registry.
  void ReportMemoryUsage() const {
    base::win::RegKey reporter_key;
    DWORD memory_used = 0;
    if (reporter_key.Open(HKEY_CURRENT_USER, registry_key_.c_str(),
                          KEY_QUERY_VALUE | KEY_SET_VALUE) != ERROR_SUCCESS ||
        reporter_key.ReadValueDW(chrome_cleaner::kMemoryUsedValueName,
                                 &memory_used) != ERROR_SUCCESS) {
      return;
    }
    RecordMemoryKBHistogram(kMemoryUsedMetricName, memory_used);
    reporter_key.DeleteValue(chrome_cleaner::kMemoryUsedValueName);
  }

  // Reports the SwReporter run time with UMA both as reported by the tool via
  // the registry and as measured by |ReporterRunner|.
  void ReportRuntime(const base::TimeDelta& reporter_running_time) const {
    RecordLongTimesHistogram("SoftwareReporter.RunningTimeAccordingToChrome",
                             reporter_running_time);

    // TODO(b/641081): This should only have KEY_QUERY_VALUE and KEY_SET_VALUE.
    base::win::RegKey reporter_key;
    if (reporter_key.Open(HKEY_CURRENT_USER, registry_key_.c_str(),
                          KEY_ALL_ACCESS) != ERROR_SUCCESS) {
      RecordEnumerationHistogram(
          kRunningTimeErrorMetricName,
          REPORTER_RUNNING_TIME_ERROR_REGISTRY_KEY_INVALID,
          REPORTER_RUNNING_TIME_ERROR_MAX);
      return;
    }

    bool has_start_time = false;
    int64_t start_time_value = 0;
    if (reporter_key.HasValue(chrome_cleaner::kStartTimeValueName) &&
        reporter_key.ReadInt64(chrome_cleaner::kStartTimeValueName,
                               &start_time_value) == ERROR_SUCCESS) {
      has_start_time = true;
      reporter_key.DeleteValue(chrome_cleaner::kStartTimeValueName);
    }

    bool has_end_time = false;
    int64_t end_time_value = 0;
    if (reporter_key.HasValue(chrome_cleaner::kEndTimeValueName) &&
        reporter_key.ReadInt64(chrome_cleaner::kEndTimeValueName,
                               &end_time_value) == ERROR_SUCCESS) {
      has_end_time = true;
      reporter_key.DeleteValue(chrome_cleaner::kEndTimeValueName);
    }

    if (has_start_time && has_end_time) {
      base::TimeDelta registry_run_time =
          base::Time::FromInternalValue(end_time_value) -
          base::Time::FromInternalValue(start_time_value);
      RecordLongTimesHistogram("SoftwareReporter.RunningTime",
                               registry_run_time);
      RecordEnumerationHistogram(kRunningTimeErrorMetricName,
                                 REPORTER_RUNNING_TIME_ERROR_NO_ERROR,
                                 REPORTER_RUNNING_TIME_ERROR_MAX);
    } else if (!has_start_time && !has_end_time) {
      RecordEnumerationHistogram(kRunningTimeErrorMetricName,
                                 REPORTER_RUNNING_TIME_ERROR_MISSING_BOTH_TIMES,
                                 REPORTER_RUNNING_TIME_ERROR_MAX);
    } else if (!has_start_time) {
      RecordEnumerationHistogram(kRunningTimeErrorMetricName,
                                 REPORTER_RUNNING_TIME_ERROR_MISSING_START_TIME,
                                 REPORTER_RUNNING_TIME_ERROR_MAX);
    } else {
      DCHECK(!has_end_time);
      RecordEnumerationHistogram(kRunningTimeErrorMetricName,
                                 REPORTER_RUNNING_TIME_ERROR_MISSING_END_TIME,
                                 REPORTER_RUNNING_TIME_ERROR_MAX);
    }
  }

  // Reports the UwS scan times of the software reporter tool via UMA.
  void ReportScanTimes() const {
    base::string16 scan_times_key_path = base::StringPrintf(
        L"%ls\\%ls", registry_key_.c_str(), chrome_cleaner::kScanTimesSubKey);
    // TODO(b/641081): This should only have KEY_QUERY_VALUE and KEY_SET_VALUE.
    base::win::RegKey scan_times_key;
    if (scan_times_key.Open(HKEY_CURRENT_USER, scan_times_key_path.c_str(),
                            KEY_ALL_ACCESS) != ERROR_SUCCESS) {
      return;
    }

    base::string16 value_name;
    int uws_id = 0;
    int64_t raw_scan_time = 0;
    int num_scan_times = scan_times_key.GetValueCount();
    for (int i = 0; i < num_scan_times; ++i) {
      if (scan_times_key.GetValueNameAt(i, &value_name) == ERROR_SUCCESS &&
          base::StringToInt(value_name, &uws_id) &&
          scan_times_key.ReadInt64(value_name.c_str(), &raw_scan_time) ==
              ERROR_SUCCESS) {
        base::TimeDelta scan_time =
            base::TimeDelta::FromInternalValue(raw_scan_time);
        // We report the number of seconds plus one because it can take less
        // than one second to scan some UwS and the count passed to |AddCount|
        // must be at least one.
        RecordSparseHistogramCount(kScanTimesMetricName, uws_id,
                                   scan_time.InSeconds() + 1);
      }
    }
    // Clean up by deleting the scan times key, which is a subkey of the main
    // reporter key.
    scan_times_key.Close();
    base::win::RegKey reporter_key;
    if (reporter_key.Open(HKEY_CURRENT_USER, registry_key_.c_str(),
                          KEY_ENUMERATE_SUB_KEYS) == ERROR_SUCCESS) {
      reporter_key.DeleteKey(chrome_cleaner::kScanTimesSubKey);
    }
  }

  void RecordReporterStep(SwReporterUmaValue value) {
    RecordEnumerationHistogram(kStepMetricName, value, SW_REPORTER_MAX);
  }

  void RecordLogsUploadEnabled(SwReporterLogsUploadsEnabled value) {
    RecordEnumerationHistogram(kLogsUploadEnabledMetricName, value,
                               REPORTER_LOGS_UPLOADS_MAX);
  }

  void RecordLogsUploadResult() {
    base::win::RegKey reporter_key;
    DWORD logs_upload_result = 0;
    if (reporter_key.Open(HKEY_CURRENT_USER, registry_key_.c_str(),
                          KEY_QUERY_VALUE | KEY_SET_VALUE) != ERROR_SUCCESS) {
      RecordEnumerationHistogram(
          kLogsUploadResultRegistryErrorMetricName,
          REPORTER_LOGS_UPLOAD_RESULT_ERROR_REGISTRY_KEY_INVALID,
          REPORTER_LOGS_UPLOAD_RESULT_ERROR_MAX);
      return;
    }

    if (reporter_key.ReadValueDW(chrome_cleaner::kLogsUploadResultValueName,
                                 &logs_upload_result) != ERROR_SUCCESS) {
      RecordEnumerationHistogram(
          kLogsUploadResultRegistryErrorMetricName,
          REPORTER_LOGS_UPLOAD_RESULT_ERROR_VALUE_NOT_FOUND,
          REPORTER_LOGS_UPLOAD_RESULT_ERROR_MAX);
      return;
    }

    if (logs_upload_result >= kSwReporterLogsUploadResultMax) {
      RecordEnumerationHistogram(
          kLogsUploadResultRegistryErrorMetricName,
          REPORTER_LOGS_UPLOAD_RESULT_ERROR_VALUE_OUT_OF_BOUNDS,
          REPORTER_LOGS_UPLOAD_RESULT_ERROR_MAX);
      return;
    }

    RecordEnumerationHistogram(kLogsUploadResultMetricName,
                               static_cast<Sample>(logs_upload_result),
                               kSwReporterLogsUploadResultMax);
    reporter_key.DeleteValue(chrome_cleaner::kLogsUploadResultValueName);
    RecordEnumerationHistogram(kLogsUploadResultRegistryErrorMetricName,
                               REPORTER_LOGS_UPLOAD_RESULT_ERROR_NO_ERROR,
                               REPORTER_LOGS_UPLOAD_RESULT_ERROR_MAX);
  }

 private:
  using Sample = base::HistogramBase::Sample;

  static constexpr base::HistogramBase::Flags kUmaHistogramFlag =
      base::HistogramBase::kUmaTargetedHistogramFlag;

  // Helper functions to record histograms with an optional suffix added to the
  // histogram name. The UMA_HISTOGRAM macros can't be used because they
  // require a constant string.

  std::string FullName(const std::string& name) const {
    if (suffix_.empty())
      return name;
    return base::StringPrintf("%s_%s", name.c_str(), suffix_.c_str());
  }

  void RecordBooleanHistogram(const std::string& name, bool sample) const {
    auto* histogram =
        base::BooleanHistogram::FactoryGet(FullName(name), kUmaHistogramFlag);
    if (histogram)
      histogram->AddBoolean(sample);
  }

  void RecordEnumerationHistogram(const std::string& name,
                                  Sample sample,
                                  Sample boundary) const {
    // See HISTOGRAM_ENUMERATION_WITH_FLAG for the parameters to |FactoryGet|.
    auto* histogram = base::LinearHistogram::FactoryGet(
        FullName(name), 1, boundary, boundary + 1, kUmaHistogramFlag);
    if (histogram)
      histogram->Add(sample);
  }

  void RecordLongTimesHistogram(const std::string& name,
                                const base::TimeDelta& sample) const {
    // See UMA_HISTOGRAM_LONG_TIMES for the parameters to |FactoryTimeGet|.
    auto* histogram = base::Histogram::FactoryTimeGet(
        FullName(name), base::TimeDelta::FromMilliseconds(1),
        base::TimeDelta::FromHours(1), 100, kUmaHistogramFlag);
    if (histogram)
      histogram->AddTime(sample);
  }

  void RecordMemoryKBHistogram(const std::string& name, Sample sample) const {
    // See UMA_HISTOGRAM_MEMORY_KB for the parameters to |FactoryGet|.
    auto* histogram = base::Histogram::FactoryGet(FullName(name), 1000, 500000,
                                                  50, kUmaHistogramFlag);
    if (histogram)
      histogram->Add(sample);
  }

  void RecordSparseHistogram(const std::string& name, Sample sample) const {
    auto* histogram =
        base::SparseHistogram::FactoryGet(FullName(name), kUmaHistogramFlag);
    if (histogram)
      histogram->Add(sample);
  }

  void RecordSparseHistogramCount(const std::string& name,
                                  Sample sample,
                                  int count) const {
    auto* histogram =
        base::SparseHistogram::FactoryGet(FullName(name), kUmaHistogramFlag);
    if (histogram)
      histogram->AddCount(sample, count);
  }

  const std::string suffix_;
  const std::wstring registry_key_;
};

// Records the reporter step without a suffix. (For steps that are never run by
// the experimental reporter.)
void RecordReporterStepHistogram(SwReporterUmaValue value) {
  UMAHistogramReporter uma;
  uma.RecordReporterStep(value);
}

// This function is called from a worker thread to launch the SwReporter and
// wait for termination to collect its exit code. This task could be
// interrupted by a shutdown at any time, so it shouldn't depend on anything
// external that could be shut down beforehand.
int LaunchAndWaitForExitOnBackgroundThread(SwReporterInvocation invocation) {
  if (g_testing_delegate_)
    return g_testing_delegate_->LaunchReporter(invocation);
  base::Process reporter_process =
      base::LaunchProcess(invocation.command_line(), base::LaunchOptions());
  // This exit code is used to identify that a reporter run didn't happen, so
  // the result should be ignored and a rerun scheduled for the usual delay.
  int exit_code = kReporterNotLaunchedExitCode;
  UMAHistogramReporter uma(invocation.suffix());
  if (reporter_process.IsValid()) {
    uma.RecordReporterStep(SW_REPORTER_START_EXECUTION);
    bool success = reporter_process.WaitForExit(&exit_code);
    DCHECK(success);
  } else {
    uma.RecordReporterStep(SW_REPORTER_FAILED_TO_START);
  }
  return exit_code;
}

}  // namespace

namespace {

// Scans and shows the Chrome Cleaner UI if the user has not already been
// prompted in the current prompt wave.
void MaybeScanAndPrompt(SwReporterInvocationType invocation_type,
                        SwReporterInvocation reporter_invocation) {
  ChromeCleanerController* cleaner_controller =
      ChromeCleanerController::GetInstance();

  if (cleaner_controller->state() != ChromeCleanerController::State::kIdle) {
    RecordPromptNotShownWithReasonHistogram(NO_PROMPT_REASON_NOT_ON_IDLE_STATE);
    return;
  }

  Browser* browser = chrome::FindLastActive();
  if (!browser) {
    RecordReporterStepHistogram(SW_REPORTER_NO_BROWSER);
    return;
  }

  Profile* profile = browser->profile();
  DCHECK(profile);
  PrefService* prefs = profile->GetPrefs();
  DCHECK(prefs);

  // Don't show the prompt again if it's been shown before for this profile and
  // for the current variations seed. The seed preference will be updated once
  // the prompt is shown.
  const std::string incoming_seed = GetIncomingSRTSeed();
  const std::string old_seed = prefs->GetString(prefs::kSwReporterPromptSeed);
  if (!incoming_seed.empty() && incoming_seed == old_seed) {
    RecordReporterStepHistogram(SW_REPORTER_ALREADY_PROMPTED);
    RecordPromptNotShownWithReasonHistogram(NO_PROMPT_REASON_ALREADY_PROMPTED);
    return;
  }

  // TODO(crbug.com/776538): Define a delegate with default behaviour that is
  //                          overriden (instead of defined) by tests.
  // This approach is preventing makes it harder to define proper tests for
  // calls to Scan(), prompt not shown for user-initiated runs, and cleaner logs
  // uploading.
  if (g_testing_delegate_) {
    g_testing_delegate_->TriggerPrompt();
    return;
  }

  if (invocation_type ==
      SwReporterInvocationType::kUserInitiatedWithLogsAllowed) {
    reporter_invocation.set_cleaner_logs_upload_enabled(true);
  }

  cleaner_controller->Scan(reporter_invocation);
  DCHECK_EQ(ChromeCleanerController::State::kScanning,
            cleaner_controller->state());

  // If this is a periodic reporter run, then creates the dialog controller, so
  // that the user may eventually be prompted. Otherwise, all interaction
  // should be driven from the Settings page.
  if (invocation_type == SwReporterInvocationType::kPeriodicRun)
    // The dialog controller manages its own lifetime. If the controller enters
    // the kInfected state, the dialog controller will show the chrome cleaner
    // dialog to the user.
    new ChromeCleanerDialogControllerImpl(cleaner_controller);
}

base::Time Now() {
  return g_testing_delegate_ ? g_testing_delegate_->Now() : base::Time::Now();
}

}  // namespace

// This class tries to run a queue of reporters and react to their exit codes.
// It schedules subsequent runs of the queue as needed, or retries as soon as a
// browser is available when none is on first try.
class ReporterRunner {
 public:
  // Registers |invocations| to run next time TryToRunInvocationsSequence() is
  // scheduled. (And if it's not already scheduled, call it now.)
  static void ScheduleInvocations(SwReporterInvocationType invocation_type,
                                  SwReporterInvocationsSequence&& invocations) {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);

    if (!instance_) {
      instance_ = new ReporterRunner;
      ANNOTATE_LEAKING_OBJECT_PTR(instance_);
    }

    // User-initiated runs have priority over periodic runs. Invalidating the
    // weak pointers for the ReporterRunner instance will turn all posted tasks
    // into no-ops. This will also notify any pending action depending on
    // reporter results that it was interrupted.
    // Note: this will not stop a reporter process that is currently running,
    //       but once it ends, the ReporterDone callback will not be executed,
    //       and corresponding results will be ignored.
    if (IsUserInitiated(invocation_type)) {
      // The UI should block a new user-initiated run if another one is
      // currently happening. For simplicity at this point, simply block the new
      // invocation with an appropriate error result.
      if (instance_->is_currently_running_user_initiated_) {
        invocations.NotifySequenceDone(
            SwReporterInvocationResult::kNotScheduled);
        return;
      }

      instance_->weak_ptr_factory_.InvalidateWeakPtrs();
    }

    instance_->cached_parsed_invocations_ = std::move(invocations);
    if (instance_->first_run_ || IsUserInitiated(invocation_type)) {
      instance_->first_run_ = false;
      instance_->TryToRunInvocationsSequence(invocation_type);
    }
  }

 private:
  // Keeps track of last and upcoming reporter runs reporter logs uploading.
  class ReporterRunTimeInfo {
   public:
    explicit ReporterRunTimeInfo(PrefService* local_state)
        : local_state_(local_state) {
      DCHECK(local_state);

      now_ = Now();
      if (local_state_->HasPrefPath(prefs::kSwReporterLastTimeTriggered)) {
        last_time_triggered_ =
            base::Time() +
            base::TimeDelta::FromMicroseconds(
                local_state->GetInt64(prefs::kSwReporterLastTimeTriggered));
        next_trigger_ =
            last_time_triggered_ +
            base::TimeDelta::FromDays(kDaysBetweenSuccessfulSwReporterRuns);
      } else {
        next_trigger_ = now_;
      }
    }

    // Periodic runs are allowed to start if the last time the reporter ran was
    // more than |kDaysBetweenSuccessfulSwReporterRuns| days ago. As a safety
    // measure for failure recovery, also allow sending logs if the last time
    // ran in local state is incorrectly set to the future.
    bool ShouldStartPeriodicRun() {
      return next_trigger_ <= now_ || last_time_triggered_ > now_;
    }

    base::TimeDelta UpcomingReporterRun() { return next_trigger_ - now_; }

    // Allow logs uploading if logs have never been uploaded for this user,
    // or if logs have been sent at least |kSwReporterLastTimeSentReport| days
    // ago. As a safety measure for failure recovery, also allow sending logs if
    // the last upload time in local state is incorrectly set to the future.
    bool InLogsUploadPeriod() {
      if (!local_state_->HasPrefPath(prefs::kSwReporterLastTimeSentReport))
        return true;

      const base::Time last_time_sent_logs =
          base::Time() +
          base::TimeDelta::FromMicroseconds(
              local_state_->GetInt64(prefs::kSwReporterLastTimeSentReport));
      const base::Time next_time_send_logs =
          last_time_sent_logs +
          base::TimeDelta::FromDays(kDaysBetweenReporterLogsSent);
      return last_time_sent_logs > now_ || next_time_send_logs <= now_;
    }

    PrefService* local_state_ = nullptr;
    base::Time now_;
    base::Time last_time_triggered_;
    base::Time next_trigger_;
  };

  ReporterRunner() : weak_ptr_factory_(this) {}
  virtual ~ReporterRunner() {}

  // Launches the command line at the head of the queue.
  void ScheduleNextInvocation(SwReporterInvocationType invocation_type) {
    DCHECK(!invocations_currently_running_.container().empty());
    auto next_invocation = invocations_currently_running_.container().front();
    invocations_currently_running_.container().pop_front();

    AppendInvocationSpecificSwitches(invocation_type, &next_invocation);

    base::TaskRunner* task_runner =
        g_testing_delegate_ ? g_testing_delegate_->BlockingTaskRunner()
                            : blocking_task_runner_.get();
    auto launch_and_wait =
        base::Bind(&LaunchAndWaitForExitOnBackgroundThread, next_invocation);
    auto reporter_done =
        base::Bind(&ReporterRunner::ReporterDone, GetWeakPtr(), Now(),
                   invocations_currently_running_.version(), invocation_type,
                   next_invocation);
    base::PostTaskAndReplyWithResult(task_runner, FROM_HERE,
                                     std::move(launch_and_wait),
                                     std::move(reporter_done));
  }

  // This method is called on the UI thread when an invocation of the reporter
  // has completed. This is run as a task posted from an interruptible worker
  // thread so should be resilient to unexpected shutdown.
  void ReporterDone(const base::Time& reporter_start_time,
                    const base::Version& version,
                    SwReporterInvocationType invocation_type,
                    SwReporterInvocation finished_invocation,
                    int exit_code) {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);

    base::Time now = Now();
    base::TimeDelta reporter_running_time = now - reporter_start_time;

    // Don't continue the current queue of reporters if one failed to launch.
    if (exit_code == kReporterNotLaunchedExitCode)
      invocations_currently_running_ = SwReporterInvocationsSequence(version);

    // As soon as we're not running this queue, schedule the next overall queue
    // run after the regular delay. (If there was a failure it's not worth
    // retrying earlier, risking running too often if it always fails, since
    // not many users fail here.)
    if (invocations_currently_running_.container().empty()) {
      // Schedule to run again in |kDaysBetweenSuccessfulSwReporterRuns| days.
      base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
          FROM_HERE,
          base::Bind(&ReporterRunner::TryToRunInvocationsSequence, GetWeakPtr(),
                     SwReporterInvocationType::kPeriodicRun),
          base::TimeDelta::FromDays(kDaysBetweenSuccessfulSwReporterRuns));

      // Notify any pending action depending on the reporter result.
      invocations_currently_running_.NotifySequenceDone(
          ExitCodeToInvocationResult(exit_code));
      is_currently_running_user_initiated_ = false;
    } else {
      ScheduleNextInvocation(invocation_type);
    }

    // If the reporter failed to launch, do not process the results. (The exit
    // code itself doesn't need to be logged in this case because
    // SW_REPORTER_FAILED_TO_START is logged in
    // |LaunchAndWaitForExitOnBackgroundThread|.)
    if (exit_code == kReporterNotLaunchedExitCode) {
      invocations_currently_running_.NotifySequenceDone(
          SwReporterInvocationResult::kFailed);
      return;
    }

    UMAHistogramReporter uma(finished_invocation.suffix());
    uma.ReportVersion(version);
    uma.ReportExitCode(exit_code);
    uma.ReportEngineErrorCode();
    uma.ReportFoundUwS();

    PrefService* local_state = g_browser_process->local_state();
    if (local_state) {
      if (finished_invocation.BehaviourIsSupported(
              SwReporterInvocation::BEHAVIOUR_LOG_EXIT_CODE_TO_PREFS)) {
        local_state->SetInteger(prefs::kSwReporterLastExitCode, exit_code);
      }
      local_state->SetInt64(prefs::kSwReporterLastTimeTriggered,
                            now.ToInternalValue());
    }
    uma.ReportRuntime(reporter_running_time);
    uma.ReportScanTimes();
    uma.ReportMemoryUsage();
    if (finished_invocation.reporter_logs_upload_enabled())
      uma.RecordLogsUploadResult();

    if (!finished_invocation.BehaviourIsSupported(
            SwReporterInvocation::BEHAVIOUR_TRIGGER_PROMPT)) {
      RecordPromptNotShownWithReasonHistogram(
          NO_PROMPT_REASON_BEHAVIOUR_NOT_SUPPORTED);
      return;
    }

    if (!IsInSRTPromptFieldTrialGroups()) {
      // Knowing about disabled field trial is more important than reporter not
      // finding anything to remove, so check this case first.
      RecordReporterStepHistogram(SW_REPORTER_NO_PROMPT_FIELD_TRIAL);
      RecordPromptNotShownWithReasonHistogram(
          NO_PROMPT_REASON_FEATURE_NOT_ENABLED);
      return;
    }

    if (exit_code != chrome_cleaner::kSwReporterPostRebootCleanupNeeded &&
        exit_code != chrome_cleaner::kSwReporterCleanupNeeded) {
      RecordReporterStepHistogram(SW_REPORTER_NO_PROMPT_NEEDED);
      RecordPromptNotShownWithReasonHistogram(NO_PROMPT_REASON_NOTHING_FOUND);
      return;
    }

    MaybeScanAndPrompt(invocation_type, finished_invocation);
  }

  void TryToRunInvocationsSequence(SwReporterInvocationType invocation_type) {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    PrefService* local_state = g_browser_process->local_state();

    if (!cached_parsed_invocations_.version().IsValid() || !local_state) {
      // TODO(b/641081): This doesn't look right. Even on first run, the version
      // should be valid (and this is already checked in RunSwReporters). We
      // should abort if local_state is missing, but this has nothing to do
      // with |first_run_|.
      DCHECK(first_run_);
      return;
    }

    // Run a queue of reporters if none have been triggered in the last
    // |kDaysBetweenSuccessfulSwReporterRuns| days.
    ReporterRunTimeInfo reporter_time_info(local_state);
    const bool is_user_initiated = IsUserInitiated(invocation_type);
    if (!cached_parsed_invocations_.container().empty() &&
        (IsUserInitiated(invocation_type) ||
         reporter_time_info.ShouldStartPeriodicRun())) {
      in_logs_upload_period_ = reporter_time_info.InLogsUploadPeriod();
      invocations_currently_running_.ImportFrom(&cached_parsed_invocations_);
      is_currently_running_user_initiated_ = is_user_initiated;
      ScheduleNextInvocation(invocation_type);
    } else {
      // Regardless of how this run started, the next run will be scheduled
      // as periodic.
      base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
          FROM_HERE,
          base::Bind(&ReporterRunner::TryToRunInvocationsSequence, GetWeakPtr(),
                     SwReporterInvocationType::kPeriodicRun),
          reporter_time_info.UpcomingReporterRun());
    }
  }

  // Returns true if the experiment to send reporter logs is enabled, the user
  // opted into Safe Browsing extended reporting, and this queue of invocations
  // started during the logs upload interval.
  bool ShouldSendReporterLogs(SwReporterInvocationType invocation_type,
                              const SwReporterInvocation& invocation,
                              const PrefService& local_state) {
    UMAHistogramReporter uma(invocation.suffix());

    switch (invocation_type) {
      case SwReporterInvocationType::kUserInitiatedWithLogsDisallowed:
        uma.RecordLogsUploadEnabled(REPORTER_LOGS_UPLOADS_DISABLED_BY_USER);
        return false;

      case SwReporterInvocationType::kUserInitiatedWithLogsAllowed:
        uma.RecordLogsUploadEnabled(REPORTER_LOGS_UPLOADS_ENABLED_BY_USER);
        return true;

      case SwReporterInvocationType::kPeriodicRun:
        if (!SafeBrowsingExtendedReportingEnabled()) {
          uma.RecordLogsUploadEnabled(REPORTER_LOGS_UPLOADS_SBER_DISABLED);
          return false;
        }
        uma.RecordLogsUploadEnabled(
            in_logs_upload_period_ ? REPORTER_LOGS_UPLOADS_SBER_ENABLED
                                   : REPORTER_LOGS_UPLOADS_RECENTLY_SENT_LOGS);
        return in_logs_upload_period_;
    }
  }

  // Appends switches to the next invocation that depend on the user current
  // state with respect to opting into extended Safe Browsing reporting and
  // metrics and crash reporting. The invocation object is changed locally right
  // before the actual process is launched because user status can change
  // between this and the next run for this ReporterRunner object. For example,
  // the ReporterDone() callback schedules the next run for a few days later,
  // and the user might have changed settings in the meantime.
  void AppendInvocationSpecificSwitches(
      SwReporterInvocationType invocation_type,
      SwReporterInvocation* next_invocation) {
    // Add switches for users who opted into extended Safe Browsing reporting.
    PrefService* local_state = g_browser_process->local_state();
    if (local_state && ShouldSendReporterLogs(invocation_type, *next_invocation,
                                              *local_state)) {
      next_invocation->set_reporter_logs_upload_enabled(true);
      AddSwitchesForExtendedReportingUser(next_invocation);
      // Set the local state value before the first attempt to run the
      // reporter, because we only want to upload logs once in the window
      // defined by |kDaysBetweenReporterLogsSent|. If we set with other local
      // state values after the reporter runs, we could send logs again too
      // quickly (for example, if Chrome stops before the reporter finishes).
      local_state->SetInt64(prefs::kSwReporterLastTimeSentReport,
                            Now().ToInternalValue());
    }

    if (ChromeMetricsServiceAccessor::IsMetricsAndCrashReportingEnabled()) {
      next_invocation->command_line().AppendSwitch(
          chrome_cleaner::kEnableCrashReportingSwitch);
    }

    const std::string group_name = GetSRTFieldTrialGroupName();
    if (!group_name.empty()) {
      next_invocation->command_line().AppendSwitchASCII(
          chrome_cleaner::kSRTPromptFieldTrialGroupNameSwitch, group_name);
    }
  }

  // Adds switches to be sent to the Software Reporter when the user opted into
  // extended Safe Browsing reporting and is not incognito.
  void AddSwitchesForExtendedReportingUser(SwReporterInvocation* invocation) {
    invocation->command_line().AppendSwitch(
        chrome_cleaner::kExtendedSafeBrowsingEnabledSwitch);
    invocation->command_line().AppendSwitchASCII(
        chrome_cleaner::kChromeVersionSwitch, version_info::GetVersionNumber());
    invocation->command_line().AppendSwitchNative(
        chrome_cleaner::kChromeChannelSwitch,
        base::IntToString16(ChannelAsInt()));
  }

  SwReporterInvocationResult ExitCodeToInvocationResult(int exit_code) {
    LOG(ERROR) << "ExitCodeToInvocationResult: " << exit_code;
    switch (exit_code) {
      case chrome_cleaner::kSwReporterCleanupNeeded:
      // Fallthrough.
      case chrome_cleaner::kSwReporterPostRebootCleanupNeeded:
        return SwReporterInvocationResult::kCleanupNeeded;

      case chrome_cleaner::kSwReporterNothingFound:
      // Fallthrough.
      case chrome_cleaner::kSwReporterNonRemovableOnly:
      // Fallthrough.
      case chrome_cleaner::kSwReporterSuspiciousOnly:
        return SwReporterInvocationResult::kNothingFound;

      case chrome_cleaner::kSwReporterTimeoutWithUwS:
      // Fallthrough.
      case chrome_cleaner::kSwReporterTimeoutWithoutUwS:
        return SwReporterInvocationResult::kTimedOut;
    }

    return SwReporterInvocationResult::kFailed;
  }

  base::WeakPtr<ReporterRunner> GetWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

  bool first_run_ = true;

  // The queue of invocations that are currently running.
  SwReporterInvocationsSequence invocations_currently_running_;

  // True while this is processing user-initiated runs.
  bool is_currently_running_user_initiated_ = false;

  // The invocations to run next time the SwReporter is run.
  SwReporterInvocationsSequence cached_parsed_invocations_;

  scoped_refptr<base::TaskRunner> blocking_task_runner_ =
      base::CreateTaskRunnerWithTraits(
          // LaunchAndWaitForExitOnBackgroundThread() creates (MayBlock()) and
          // joins (WithBaseSyncPrimitives()) a process.
          {base::MayBlock(), base::WithBaseSyncPrimitives(),
           base::TaskPriority::BACKGROUND,
           base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN});

  // This will be true if the current queue of invocations started at a time
  // when logs should be uploaded.
  bool in_logs_upload_period_ = false;

  // A single leaky instance.
  static ReporterRunner* instance_;

  base::WeakPtrFactory<ReporterRunner> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ReporterRunner);
};

ReporterRunner* ReporterRunner::instance_ = nullptr;

bool IsUserInitiated(SwReporterInvocationType invocation_type) {
  return invocation_type ==
             SwReporterInvocationType::kUserInitiatedWithLogsAllowed ||
         invocation_type ==
             SwReporterInvocationType::kUserInitiatedWithLogsDisallowed;
}

SwReporterInvocation::SwReporterInvocation(
    const base::CommandLine& command_line)
    : command_line_(command_line) {}

SwReporterInvocation::SwReporterInvocation(const SwReporterInvocation& other)
    : command_line_(other.command_line_),
      supported_behaviours_(other.supported_behaviours_),
      suffix_(other.suffix_) {}

SwReporterInvocation& SwReporterInvocation::WithSuffix(
    const std::string& suffix) {
  suffix_ = suffix;
  return *this;
}

SwReporterInvocation& SwReporterInvocation::WithSupportedBehaviours(
    Behaviours supported_behaviours) {
  supported_behaviours_ = supported_behaviours;
  return *this;
}

bool SwReporterInvocation::operator==(const SwReporterInvocation& other) const {
  return command_line_.argv() == other.command_line_.argv() &&
         supported_behaviours_ == other.supported_behaviours_ &&
         suffix_ == other.suffix_;
}

const base::CommandLine& SwReporterInvocation::command_line() const {
  return command_line_;
}

base::CommandLine& SwReporterInvocation::command_line() {
  return command_line_;
}

SwReporterInvocation::Behaviours SwReporterInvocation::supported_behaviours()
    const {
  return supported_behaviours_;
}

bool SwReporterInvocation::BehaviourIsSupported(
    SwReporterInvocation::Behaviours intended_behaviour) const {
  return (supported_behaviours_ & intended_behaviour) != 0;
}

std::string SwReporterInvocation::suffix() const {
  return suffix_;
}

bool SwReporterInvocation::reporter_logs_upload_enabled() const {
  return reporter_logs_upload_enabled_;
}

void SwReporterInvocation::set_reporter_logs_upload_enabled(
    bool reporter_logs_upload_enabled) {
  reporter_logs_upload_enabled_ = reporter_logs_upload_enabled;
}

bool SwReporterInvocation::cleaner_logs_upload_enabled() const {
  return cleaner_logs_upload_enabled_;
}

void SwReporterInvocation::set_cleaner_logs_upload_enabled(
    bool cleaner_logs_upload_enabled) {
  cleaner_logs_upload_enabled_ = cleaner_logs_upload_enabled;
}

SwReporterInvocationsSequence::SwReporterInvocationsSequence(
    const base::Version& version,
    const Container& container,
    OnReporterSequenceDone on_sequence_done)
    : version_(version),
      container_(container),
      on_sequence_done_(std::move(on_sequence_done)) {}

SwReporterInvocationsSequence::SwReporterInvocationsSequence(
    SwReporterInvocationsSequence&& queue)
    : version_(std::move(queue.version_)),
      container_(std::move(queue.container_)),
      on_sequence_done_(std::move(queue.on_sequence_done_)) {}

SwReporterInvocationsSequence::~SwReporterInvocationsSequence() {
  NotifySequenceDone(SwReporterInvocationResult::kInterrupted);
}

void SwReporterInvocationsSequence::operator=(
    SwReporterInvocationsSequence&& queue) {
  version_ = std::move(queue.version_);
  container_ = std::move(queue.container_);
  on_sequence_done_ = std::move(queue.on_sequence_done_);
}

void SwReporterInvocationsSequence::ImportFrom(
    SwReporterInvocationsSequence* source) {
  version_ = source->version_;
  container_ = source->container_;
  on_sequence_done_ = std::move(source->on_sequence_done_);
}

void SwReporterInvocationsSequence::NotifySequenceDone(
    SwReporterInvocationResult result) {
  if (on_sequence_done_)
    std::move(on_sequence_done_).Run(result);
}

base::Version SwReporterInvocationsSequence::version() const {
  return version_;
}

const SwReporterInvocationsSequence::Container&
SwReporterInvocationsSequence::container() const {
  return container_;
}

SwReporterInvocationsSequence::Container&
SwReporterInvocationsSequence::container() {
  return container_;
}

void RunSwReporters(SwReporterInvocationType invocation_type,
                    SwReporterInvocationsSequence&& invocations) {
  DCHECK(!invocations.container().empty());
  ReporterRunner::ScheduleInvocations(invocation_type, std::move(invocations));
}

bool ReporterFoundUws() {
  PrefService* local_state = g_browser_process->local_state();
  if (!local_state)
    return false;
  int exit_code = local_state->GetInteger(prefs::kSwReporterLastExitCode);
  return exit_code == chrome_cleaner::kSwReporterCleanupNeeded;
}

void SetSwReporterTestingDelegate(SwReporterTestingDelegate* delegate) {
  g_testing_delegate_ = delegate;
}

}  // namespace safe_browsing
