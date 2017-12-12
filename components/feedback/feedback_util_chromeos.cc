// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/feedback/feedback_util_chromeos.h"

#include <memory>
#include <string>

#include "base/bind.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/debug_daemon_client.h"
#include "components/strings/grit/components_chromium_strings.h"
#include "third_party/cros_system_api/dbus/debugd/dbus-constants.h"
#include "ui/base/l10n/l10n_util.h"

namespace feedback_util {

namespace {

void OnLogsReceived(const GetSpecialLogsCallback& callback,
                    bool succeeded,
                    const std::map<std::string, std::string>& logs) {
  if (succeeded) {
    callback.Run(logs);
  } else {
    LOG(WARNING) << "GetSpecialFeedbackLogs call failed";
    callback.Run(std::map<std::string, std::string>());
  }
}

}  // namespace

std::unique_ptr<std::string> GetDisplayValue(const std::string& key) {
  if (key == debugd::kIwlwifiDumpKey) {
    return std::make_unique<std::string>(
        l10n_util::GetStringUTF8(IDS_IWLWIFI_DEBUG_DUMP_EXPLAINER));
  }

  return nullptr;
}

void GetSpecialLogs(const GetSpecialLogsCallback& callback) {
  chromeos::DebugDaemonClient* debugd_client =
      chromeos::DBusThreadManager::Get()->GetDebugDaemonClient();
  debugd_client->GetScrubbedSpecialLogs(
      base::Bind(&OnLogsReceived, callback));
}

}  // namespace feedback_util
