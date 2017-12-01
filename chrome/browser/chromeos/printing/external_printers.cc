// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/printing/external_printers.h"

#include <set>
#include <utility>

#include "base/json/json_reader.h"
#include "base/memory/ptr_util.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/sequence_checker.h"
#include "base/stl_util.h"
#include "base/values.h"
#include "chromeos/printing/printer_translator.h"

namespace chromeos {
namespace {

class ExternalPrintersImpl : public ExternalPrinters {
 public:
  ExternalPrintersImpl() { LOG(WARNING) << "External Printers Constructed"; }
  ~ExternalPrintersImpl() override = default;

  // Resets the printer state fields.
  void ClearData() override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    LOG(WARNING) << "Clearing State";
    bool notify = !printers_.empty();
    policy_retrieved_ = false;
    printers_ready_ = false;
    printers_.clear();
    all_printers_.clear();

    if (notify) {
      Notify();
    }
  }

  void SetData(std::unique_ptr<std::string> data) override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    base::JSONReader reader;
    std::unique_ptr<base::Value> json_blob = reader.ReadToValue(*data);
    // It's not valid JSON.  Invalidate config.
    if (!json_blob) {
      LOG(WARNING) << "Failed to parse external policy";
      ClearData();
      return;
    }

    for (const base::Value& val : json_blob->GetList()) {
      // TODO(skau): Convert to the new Value APIs.
      const base::DictionaryValue* printer_dict;
      if (!val.GetAsDictionary(&printer_dict)) {
        LOG(WARNING) << "Entry is not a dictionary.";
        continue;
      }

      LOG(WARNING) << "PRINTER: " << *printer_dict;
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

  void AddObserver(Observer* observer) override {
    observers_.AddObserver(observer);
  }

  void RemoveObserver(Observer* observer) override {
    observers_.RemoveObserver(observer);
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

    // We assume something changed.  Notify now.
    Notify();
  }

  void Notify() {
    LOG(WARNING) << "Notify of change";
    for (auto& observer : observers_) {
      observer.OnPrintersChanged();
    }
  }

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

  base::ObserverList<ExternalPrinters::Observer> observers_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(ExternalPrintersImpl);
};
}  // namespace

ExternalPrinters::ExternalPrinters() = default;

// static
std::unique_ptr<ExternalPrinters> ExternalPrinters::Create() {
  return base::MakeUnique<ExternalPrintersImpl>();
}

}  // namespace chromeos
