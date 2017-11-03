// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiling_host/profiling_test_driver.h"

#include "content/public/browser/browser_thread.h"
#include "content/public/common/service_manager_connection.h"

namespace profiling {

ProfilingTestDriver::ProfilingTestDriver() {}
ProfilingTestDriver::~ProfilingTestDriver() {}

bool ProfilingTestDriver::RunTest(const Options& options) {
  options_ = options;

  if (!content::BrowserThread::CurrentlyOn(content::BrowserThread::UI)) {
    LOG(ERROR) << "The ProfilingTestDriver must be called on the UI thread.";
    return false;
  }

  // The only thing to test for Mode::kNone is that profiling hasn't started.
  if (options_.mode == ProfilingProcessHost::Mode::kNone) {
    if (ProfilingProcessHost::has_started()) {
      LOG(ERROR) << "Profiling should not have started";
      return false;
    }
    return true;
  }

  if (!CheckOrStartProfiling())
    return false;

  return true;
}

bool ProfilingTestDriver::CheckOrStartProfiling() {
  if (options_.profiling_already_started) {
    if (ProfilingProcessHost::has_started())
      return true;
    LOG(ERROR) << "Profiling should have been started, but wasn't";
    return false;
  }

  content::ServiceManagerConnection* connection = content::ServiceManagerConnection::GetForProcess();
  if (!connection) {
    LOG(ERROR) << "A ServiceManagerConnection was not available for the current process.";
    return false;
  }

  ProfilingProcessHost::Start(connection, options_.mode);
  return true;
}

}  // namespace profiling
