// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SAFE_BROWSING_CHROME_CLEANER_REPORTER_RUNNER_WIN_H_
#define CHROME_BROWSER_SAFE_BROWSING_CHROME_CLEANER_REPORTER_RUNNER_WIN_H_

#include <limits.h>
#include <stdint.h>

#include <memory>
#include <string>

#include "base/command_line.h"
#include "base/containers/circular_deque.h"
#include "base/time/time.h"
#include "base/version.h"

namespace base {
class TaskRunner;
class Version;
}

namespace safe_browsing {

// A special exit code identifying a failure to run the reporter.
const int kReporterNotLaunchedExitCode = INT_MAX;

// The number of days to wait before triggering another reporter run.
const int kDaysBetweenSuccessfulSwReporterRuns = 7;
// The number of days to wait before sending out reporter logs.
const int kDaysBetweenReporterLogsSent = 7;

// Identifies if an invocation was created during periodic reporter runs
// or because the user explicitly initiated a cleanup. The invocation type
// controls if a prompt dialog will be shown to the user and under what
// conditions logs may be uploaded to Google.
enum class SwReporterInvocationType {
  // Periodic runs of the reporter are initiated by Chrome after startup.
  // If removable unwanted software is found the user may be prompted to
  // run the Chrome Cleanup tool. Logs from the software reporter will only
  // be uploaded if the user has opted-into SBER2 and if they unwanted
  // software is found on the system. The cleaner process in scanning mode
  // will not upload logs.
  kPeriodicRun,
  // User-initiated runs in which the user has opted-out of sending details
  // to Google. Those runs are intended to be completely driven from the
  // Settings page, so a prompt dialog will not be shown to the user if
  // removable unwanted software is found. Logs will not be uploaded from
  // either the reporter nor the cleaner in scanning mode (which will only
  // run if unwanted software is found by the reporter), regardless of
  // any permissions the user may have previously opted into. More
  // specifically, logs will not be uploaded from the reporter even if the
  // user has opted into SBER2, and cleaner logs will not be uploaded.
  kUserInitiatedWithLogsDisallowed,
  // User-initiated runs in which the user has not opted-out of sending
  // details to Google. Those runs are intended to be completely driven from
  // the Settings page, so a prompt dialog will not be shown to the user if
  // removable unwanted software is found. Logs may be uploaded from both
  // the reporter and the cleaner in scanning mode (which will only run if
  // unwanted software if found by the reporter).
  kUserInitiatedWithLogsAllowed,
};

bool IsUserInitiated(SwReporterInvocationType invocation_type);

// Parameters used to invoke the sw_reporter component.
class SwReporterInvocation {
 public:
  // Flags to control behaviours the Software Reporter should support by
  // default. These flags are set in the Reporter installer, and experimental
  // versions of the reporter will turn on the behaviours that are not yet
  // supported.
  using Behaviours = uint32_t;
  enum : Behaviours {
    BEHAVIOUR_LOG_EXIT_CODE_TO_PREFS = 0x2,
    BEHAVIOUR_TRIGGER_PROMPT = 0x4,

    BEHAVIOUR_ENABLED_BY_DEFAULT =
        BEHAVIOUR_LOG_EXIT_CODE_TO_PREFS | BEHAVIOUR_TRIGGER_PROMPT,
  };

  SwReporterInvocation(const base::CommandLine& command_line);
  SwReporterInvocation(const SwReporterInvocation& invocation);

  // Fluent interface methods, intended to be used during initialization.
  // Sample usage:
  //   auto invocation = SwReporterInvocation(command_line)
  //       .WithSuffix("MySuffix")
  //       .WithSupportedBehaviours(
  //           SwReporterInvocation::Behaviours::BEHAVIOUR_TRIGGER_PROMPT);
  SwReporterInvocation& WithSuffix(const std::string& suffix);
  SwReporterInvocation& WithSupportedBehaviours(
      Behaviours supported_behaviours);

  bool operator==(const SwReporterInvocation& other) const;

  const base::CommandLine& command_line() const;
  base::CommandLine& command_line();

  Behaviours supported_behaviours() const;
  bool BehaviourIsSupported(Behaviours intended_behaviour) const;

  std::string suffix() const;

  // Indicates if the invocation type allows logs from be uploaded by the
  // reporter process.
  bool reporter_logs_upload_enabled() const;
  void set_reporter_logs_upload_enabled(bool reporter_logs_upload_enabled);

  // Indicates if the invocation type allows logs uploading from the cleaner
  // process in scanning mode.
  bool cleaner_logs_upload_enabled() const;
  void set_cleaner_logs_upload_enabled(bool cleaner_logs_upload_enabled);

 private:
  base::CommandLine command_line_;

  Behaviours supported_behaviours_ = 0;

  // Experimental versions of the reporter will write metrics to registry keys
  // ending in |suffix_|. Those metrics should be copied to UMA histograms also
  // ending in |suffix_|. For the canonical version, |suffix_| will be empty.
  std::string suffix_;

  bool reporter_logs_upload_enabled_ = false;
  bool cleaner_logs_upload_enabled_ = false;
};

class SwReporterInvocationsSequence {
 public:
  using Container = base::circular_deque<SwReporterInvocation>;

  SwReporterInvocationsSequence(const base::Version& version = base::Version(),
                                const Container& container = Container());
  SwReporterInvocationsSequence(const SwReporterInvocationsSequence& queue);
  virtual ~SwReporterInvocationsSequence();

  base::Version version() const;

  const Container& container() const;
  Container& container();

 private:
  base::Version version_;
  Container container_;
};

enum class SwReporterInvocationResult {
  kInterrupted,
  kTimedOut,
  kFailed,
  kNothingFound,
  kCleanupNeeded,
};

// Called when all reporter invocations have completed, with a boolean
// indicating if they succeeded.
using OnReporterDoneCallback =
    base::OnceCallback<void(SwReporterInvocationResult result)>;

// Tries to run the sw_reporter component, and then schedule the next try. If
// called multiple times, then multiple sequences of trying to run will happen,
// yet only one SwReporterInvocationsSequence will actually run in the period of
// |kDaysBetweenSuccessfulSwReporterRuns| days.
//
// Each "run" of the sw_reporter component may aggregate the results of several
// executions of the tool with different command lines. |invocations| is the
// queue of SwReporters to execute as a single "run". When a new try is
// scheduled the entire queue is executed.
//
// |version| is the version of the tool that will run.
void RunSwReporters(SwReporterInvocationType invocation_type,
                    const SwReporterInvocationsSequence& invocations,
                    OnReporterDoneCallback on_reporter_done);

// Returns true iff Local State is successfully accessed and indicates the most
// recent Reporter run terminated with an exit code indicating the presence of
// UwS.
bool ReporterFoundUws();

// Returns true iff a valid registry key for the SRT Cleaner exists, and that
// key is nonempty.
// TODO(tmartino): Consider changing to check whether the user has recently
// run the cleaner, rather than checking if they've run it at all.
bool UserHasRunCleaner();

// A delegate used by tests to implement test doubles (e.g., stubs, fakes, or
// mocks).
class SwReporterTestingDelegate {
 public:
  virtual ~SwReporterTestingDelegate() {}

  // Invoked by tests in place of base::LaunchProcess.
  virtual int LaunchReporter(const SwReporterInvocation& invocation) = 0;

  // Invoked by tests in place of the actual prompting logic.
  // See MaybeFetchSRT().
  virtual void TriggerPrompt() = 0;

  // Invoked by tests to override the current time.
  // See Now() in reporter_runner_win.cc.
  virtual base::Time Now() const = 0;

  // A task runner used to spawn the reporter process (which blocks).
  // See ReporterRunner::ScheduleNextInvocation().
  virtual base::TaskRunner* BlockingTaskRunner() const = 0;
};

// Set a delegate for testing. The implementation will not take ownership of
// |delegate| - it must remain valid until this function is called again to
// reset the delegate. If |delegate| is nullptr, any previous delegate is
// cleared.
void SetSwReporterTestingDelegate(SwReporterTestingDelegate* delegate);

}  // namespace safe_browsing

#endif  // CHROME_BROWSER_SAFE_BROWSING_CHROME_CLEANER_REPORTER_RUNNER_WIN_H_
