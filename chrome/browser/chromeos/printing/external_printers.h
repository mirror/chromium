// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_PRINTING_EXTERNAL_PRINTERS_H_
#define CHROME_BROWSER_CHROMEOS_PRINTING_EXTERNAL_PRINTERS_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "chrome/browser/chromeos/policy/cloud_external_data_policy_observer.h"
#include "chromeos/printing/printer_configuration.h"

namespace chromeos {

// Names of the external printer policies.
struct ExternalPrinterPolicies {
  ExternalPrinterPolicies();
  ExternalPrinterPolicies(const ExternalPrinterPolicies& other);
  ~ExternalPrinterPolicies();

  std::string configuration_file;
  std::string access_mode;
  std::string blacklist;
  std::string whitelist;
};

// Manages download and parsing of the external policy printer configuration and
// enforces restricitons.
class ExternalPrinters
    : public policy::CloudExternalDataPolicyObserver::Delegate {
 public:
  // Choose the policy for printer access.
  enum AccessMode { UNSET, BLACKLIST_ONLY, WHITELIST_ONLY, ALL_ACCESS };

  // Creates a handler for external printer policies for |policy_name|.
  static std::unique_ptr<ExternalPrinters> Create(
      const ExternalPrinterPolicies& policy_names);

  ~ExternalPrinters() override {}

  // policy::CloudExternalDataPolicyObserver::Delegate overrides:
  void OnExternalDataSet(const std::string& policy,
                         const std::string& user_id) override = 0;
  void OnExternalDataCleared(const std::string& policy,
                             const std::string& user_id) override = 0;
  void OnExternalDataFetched(const std::string& policy,
                             const std::string& user_id,
                             std::unique_ptr<std::string> data) override = 0;

  virtual void SetAccessMode(AccessMode mode) = 0;
  virtual void SetBlacklist(const std::vector<std::string>& blacklist) = 0;
  virtual void SetWhitelist(const std::vector<std::string>& whitelist) = 0;

  // Returns true if the printer configuration has been downloaded and parsed.
  virtual bool IsPolicySet() const = 0;

  // Returns all the printers available from the policy.
  virtual const std::vector<Printer>& GetPrinters() const = 0;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_PRINTING_EXTERNAL_PRINTERS_H_
