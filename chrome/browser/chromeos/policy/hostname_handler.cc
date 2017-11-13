// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/hostname_handler.h"

#include "base/bind.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/policy/browser_policy_connector_chromeos.h"
#include "chromeos/network/device_state.h"
#include "chromeos/network/network_handler.h"
#include "chromeos/network/network_state.h"
#include "chromeos/network/network_state_handler.h"
#include "chromeos/settings/cros_settings_names.h"
#include "chromeos/settings/cros_settings_provider.h"
#include "chromeos/system/statistics_provider.h"

namespace {

const char* kAssetIDPlaceholder = "ASSET_ID";
const char* kSerialNumPlaceholder = "SERIAL_NUM";
const char* kMACAddressPlaceholder = "MAC_ADDR";

void ReplaceAll(std::string* str,
                const std::string& from,
                const std::string& to) {
  std::size_t start = 0;
  while ((start = str->find(from, start)) != std::string::npos) {
    str->replace(start, from.length(), to);
    start += to.length();
  }
}

// As per RFC 1035, hostname should be 63 characters or less.
const int kMaxHostnameLength = 63;

bool IsValidHostnameCharacter(char c, bool is_first_char) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') || (!is_first_char && c == '-') || c == '_';
}

bool IsValidHostname(const std::string& hostname) {
  if (hostname.size() > kMaxHostnameLength)
    return false;
  bool is_first_char = true;
  for (const char& c : hostname) {
    if (!IsValidHostnameCharacter(c, is_first_char))
      return false;
    is_first_char = false;
  }
  return true;
}

}  // namespace

namespace policy {

HostnameHandler::HostnameHandler(chromeos::CrosSettings* cros_settings)
    : cros_settings_(cros_settings), weak_factory_(this) {
  policy_subscription_ = cros_settings_->AddSettingsObserver(
      chromeos::kDeviceHostnameTemplate,
      base::Bind(&HostnameHandler::OnDeviceHostnamePropertyChanged,
                 weak_factory_.GetWeakPtr()));

  // Fire it once so we're sure we get an invocation on startup.
  OnDeviceHostnamePropertyChanged();
}

HostnameHandler::~HostnameHandler() {}

void HostnameHandler::OnDeviceHostnamePropertyChanged() {
  chromeos::CrosSettingsProvider::TrustedStatus status =
      cros_settings_->PrepareTrustedValues(
          base::Bind(&HostnameHandler::OnDeviceHostnamePropertyChanged,
                     weak_factory_.GetWeakPtr()));
  if (status != chromeos::CrosSettingsProvider::TRUSTED)
    return;

  std::string hostname_template;
  cros_settings_->GetString(chromeos::kDeviceHostnameTemplate,
                            &hostname_template);

  // ASSET_ID, SERIAL_NUM or MAC_ADDR
  std::string hostname = hostname_template;

  std::string serial = chromeos::system::StatisticsProvider::GetInstance()
                           ->GetEnterpriseMachineID();

  std::string assetID = g_browser_process->platform_part()
                            ->browser_policy_connector_chromeos()
                            ->GetDeviceAssetID();

  chromeos::NetworkStateHandler* handler =
      chromeos::NetworkHandler::Get()->network_state_handler();

  std::string mac = "MAC_unknown";
  const chromeos::NetworkState* network = handler->DefaultNetwork();
  if (network) {
    const chromeos::DeviceState* device =
        handler->GetDeviceState(network->device_path());
    if (device) {
      mac = device->mac_address();
      ReplaceAll(&mac, ":", "");
    }
  }

  ReplaceAll(&hostname, kAssetIDPlaceholder, assetID);
  ReplaceAll(&hostname, kSerialNumPlaceholder, serial);
  ReplaceAll(&hostname, kMACAddressPlaceholder, mac);

  if (IsValidHostname(hostname)) {
    handler->SetHostname(hostname);
  } else {
    handler->SetHostname("");
  }
}

}  // namespace policy
