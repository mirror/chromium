// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/printing/external_printers.h"

#include <set>
#include <utility>

#include "base/json/json_reader.h"
#include "base/memory/ptr_util.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/stl_util.h"
#include "base/values.h"
#include "chromeos/printing/printer_translator.h"

namespace chromeos {
namespace {
class ExternalPrintersImpl : public ExternalPrinters {
 public:
  explicit ExternalPrintersImpl(const ExternalPrinterPolicies& policy_names)
      : policy_names_(policy_names),
        blacklist_(),
        whitelist_(),
        all_printers_(),
        printers_() {}
  ~ExternalPrintersImpl() override {}

  // policy::CloudExternalDataPolicyObserver::Delegate overrides:
  void OnExternalDataSet(const std::string& policy,
                         const std::string& user_id) override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    DCHECK_EQ(policy_names_.configuration_file, policy);
    // We need to wait for the new policy file.
    ClearAll();
  }

  void OnExternalDataCleared(const std::string& policy,
                             const std::string& user_id) override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    DCHECK_EQ(policy_names_.configuration_file, policy);
    // Policy has been cleared.  Start watiting again.
    ClearAll();
  }

  void OnExternalDataFetched(const std::string& policy,
                             const std::string& user_id,
                             std::unique_ptr<std::string> data) override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    DCHECK_EQ(policy_names_.configuration_file, policy);
    // TODO(skau): How safe is this?
    base::JSONReader reader;
    std::unique_ptr<base::Value> json_blob = reader.ReadToValue(*data);
    // It's not valid JSON.  Invalidate config.
    if (!json_blob) {
      LOG(WARNING) << "Failed to parse external policy for "
                   << policy_names_.configuration_file;
      LOG(ERROR) << reader.GetErrorMessage();
      ClearAll();
      return;
    }

    for (const base::Value& val : json_blob->GetList()) {
      // TODO(skau): Convert to the new Value APIs.
      const base::DictionaryValue* printer_dict;
      if (!val.GetAsDictionary(&printer_dict)) {
        LOG(WARNING) << "Entry is not a dictionary.";
        continue;
      }

      auto printer = RecommendedPrinterToPrinter(*printer_dict);
      if (!printer) {
        LOG(WARNING) << "Failed to parse printer configuration.";
        continue;
      }
      all_printers_.push_back(std::move(printer));
    }

    policy_retrieved_ = true;
    RecomputePrinters();
  }

  void SetAccessMode(AccessMode mode) override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    mode_ = mode;
    RecomputePrinters();
  }

  void SetBlacklist(const std::vector<std::string>& blacklist) override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    blacklist_.clear();
    blacklist_.insert(blacklist.begin(), blacklist.end());
    has_blacklist_ = true;
    RecomputePrinters();
  }

  void SetWhitelist(const std::vector<std::string>& whitelist) override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    whitelist_.clear();
    whitelist_.insert(whitelist.begin(), whitelist.end());
    has_whitelist_ = true;
    RecomputePrinters();
  }

  // Returns true if the printer configuration has been downloaded and parsed.
  bool IsPolicySet() const override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    return printers_ready_;
  }

  // Returns all the printers available from the policy.
  const std::vector<Printer>& GetPrinters() const override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    return printers_;
  }

 private:
  // Resets the printer state fields.
  void ClearAll() {
    policy_retrieved_ = false;
    printers_ready_ = false;
    printers_.clear();
    all_printers_.clear();
  }

  // Returns true if we can compute a valid whitelist.
  bool IsWhitelistReady() const {
    return mode_ == AccessMode::WHITELIST_ONLY && has_whitelist_;
  }

  // Returns true if we can compute a valid blacklist.
  bool IsBlacklistReady() const {
    return mode_ == AccessMode::BLACKLIST_ONLY && has_blacklist_;
  }

  void RecomputePrinters() {
    printers_ready_ =
        policy_retrieved_ && (IsWhitelistReady() || IsBlacklistReady() ||
                              (mode_ == AccessMode::ALL_ACCESS));
    if (!printers_ready_) {
      return;
    }

    // Drop all printers, we're recomputing.
    printers_.clear();
    switch (mode_) {
      case UNSET:
        NOTREACHED();
        break;
      case WHITELIST_ONLY:
        for (const auto& printer : all_printers_) {
          if (base::ContainsKey(whitelist_, printer->id())) {
            printers_.push_back(*printer);
          }
        }
        break;
      case BLACKLIST_ONLY:
        for (const auto& printer : all_printers_) {
          if (!base::ContainsKey(blacklist_, printer->id())) {
            printers_.push_back(*printer);
          }
        }
        break;
      case ALL_ACCESS:
        for (const auto& printer : all_printers_) {
          printers_.push_back(*printer);
        }
        break;
    }
  }

  ExternalPrinterPolicies policy_names_;
  // True if all necessary information has been set to compute the set of
  // printers.
  bool printers_ready_ = false;
  // Only true after the external policy has been downloaded.
  bool policy_retrieved_ = false;

  AccessMode mode_ = UNSET;
  bool has_blacklist_ = false;
  std::set<std::string> blacklist_;
  bool has_whitelist_ = false;
  std::set<std::string> whitelist_;

  // Cache of the parsed printer configuration file.
  std::vector<std::unique_ptr<Printer>> all_printers_;
  // The computed set of printers.
  std::vector<Printer> printers_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(ExternalPrintersImpl);
};
}  // namespace

ExternalPrinterPolicies::ExternalPrinterPolicies() = default;
ExternalPrinterPolicies::ExternalPrinterPolicies(
    const ExternalPrinterPolicies& other) = default;
ExternalPrinterPolicies::~ExternalPrinterPolicies() = default;

// static
std::unique_ptr<ExternalPrinters> ExternalPrinters::Create(
    const ExternalPrinterPolicies& policy_names) {
  return base::MakeUnique<ExternalPrintersImpl>(policy_names);
}

}  // namespace chromeos
