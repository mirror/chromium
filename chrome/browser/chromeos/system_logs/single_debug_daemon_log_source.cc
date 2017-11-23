// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/system_logs/single_debug_daemon_log_source.h"

#include <memory>

#include "base/bind.h"
#include "base/task_runner_util.h"
#include "base/task_scheduler/post_task.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/debug_daemon_client.h"
#include "components/feedback/anonymizer_tool.h"
#include "content/public/browser/browser_thread.h"

namespace system_logs {

namespace {

using SupportedSource = SingleDebugDaemonLogSource::SupportedSource;

// Converts a logs source type to the corresponding debugd log name.
std::string GetLogName(SupportedSource source_type) {
  switch (source_type) {
    case SupportedSource::kModetest:
      return "modetest";
    case SupportedSource::kLsusb:
      return "lsusb";
    case SupportedSource::kLspci:
      return "lspci";
    case SupportedSource::kIfconfig:
      return "ifconfig";
    case SupportedSource::kUptime:
      return "uptime";
  }
  NOTREACHED();
  return "";
}

// Generates a SystemLogsResponse from |result|. This is factored into a
// separate method to enable execution in a background TaskRunner.
std::unique_ptr<SystemLogsResponse> GenerateAnonymizedResponse(
    const std::string& log_name,
    base::Optional<std::string> result) {
  std::unique_ptr<SystemLogsResponse> response(new SystemLogsResponse);
  // Return an empty result if the call to GetLog() failed.
  if (result.has_value())
    response->emplace(log_name,
                      feedback::AnonymizerTool().Anonymize(result.value()));
  return response;
}

}  // namespace

SingleDebugDaemonLogSource::SingleDebugDaemonLogSource(
    SupportedSource source_type)
    : SystemLogsSource(GetLogName(source_type)), weak_ptr_factory_(this) {}

SingleDebugDaemonLogSource::~SingleDebugDaemonLogSource() {}

void SingleDebugDaemonLogSource::Fetch(const SysLogsSourceCallback& callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(!callback.is_null());

  chromeos::DebugDaemonClient* client =
      chromeos::DBusThreadManager::Get()->GetDebugDaemonClient();

  client->GetLog(
      source_name(),
      base::BindOnce(&SingleDebugDaemonLogSource::OnFetchComplete,
                     weak_ptr_factory_.GetWeakPtr(), source_name(), callback));
}

void SingleDebugDaemonLogSource::OnFetchComplete(
    const std::string& log_name,
    const SysLogsSourceCallback& callback,
    base::Optional<std::string> result) const {
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::TaskPriority::BACKGROUND},
      base::Bind(&GenerateAnonymizedResponse, log_name, std::move(result)),
      callback);
}

}  // namespace system_logs
