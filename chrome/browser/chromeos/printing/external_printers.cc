// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/printing/external_printers.h"

#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "base/feature_list.h"
#include "base/json/json_reader.h"
#include "base/memory/ptr_util.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/sequence_checker.h"
#include "base/stl_util.h"
#include "base/task_runner_util.h"
#include "base/task_scheduler/post_task.h"
#include "base/values.h"
#include "chrome/common/chrome_features.h"
#include "chromeos/printing/printer_translator.h"

namespace chromeos {

namespace {

constexpr int kMaxRecords = 20000;

using PrinterCache = std::vector<std::unique_ptr<Printer>>;
using PrinterView = std::vector<const Printer*>;

// Parses |data|, a JSON blob, into a vector of Printers.  If |data| cannot be
// parsed, returns nullptr.  This is run off the UI thread as it could be very
// slow.
std::unique_ptr<PrinterCache> ParsePrinters(std::unique_ptr<std::string> data) {
  int error_code;
  int error_line;
  std::unique_ptr<base::Value> json_blob = base::JSONReader::ReadAndReturnError(
      *data, base::JSONParserOptions::JSON_PARSE_RFC, &error_code,
      nullptr /* error_msg_out */, &error_line);
  // It's not valid JSON.  Invalidate config.
  if (!json_blob || !json_blob->is_list()) {
    LOG(WARNING) << "Failed to parse external policy (" << error_code
                 << ") on line " << error_line;
    return nullptr;
  }

  const base::Value::ListStorage& printer_list = json_blob->GetList();
  if (printer_list.size() > kMaxRecords) {
    LOG(ERROR) << "Too many records: " << printer_list.size();
    return nullptr;
  }

  auto parsed_printers = base::MakeUnique<PrinterCache>();
  for (const base::Value& val : printer_list) {
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
    parsed_printers->push_back(std::move(printer));
  }

  return parsed_printers;
}

// Computes the effective printer list using the access mode and
// blacklist/whitelist.  Methods are required to be sequenced.  This object is
// the owner of the blacklist and whitelist.
class Restrictions {
 public:
  Restrictions() { DETACH_FROM_SEQUENCE(sequence_checker_); }
  ~Restrictions() { DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_); }

  void SetBlacklist(const std::vector<std::string>& blacklist) {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    blacklist_ = std::set<std::string>(blacklist.begin(), blacklist.end());
  }

  void SetWhitelist(const std::vector<std::string>& whitelist) {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    whitelist_ = std::set<std::string>(whitelist.begin(), whitelist.end());
  }

  // Returns the effective printer list based on |mode| from the entries in
  // |cache|.
  std::unique_ptr<PrinterView> ComputePrinters(
      ExternalPrinters::AccessMode mode,
      const PrinterCache* cache) {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    auto view = base::MakeUnique<PrinterView>();
    switch (mode) {
      case ExternalPrinters::UNSET:
        NOTREACHED();
        break;
      case ExternalPrinters::WHITELIST_ONLY:
        for (const auto& printer : *cache) {
          if (base::ContainsKey(whitelist_, printer->id())) {
            view->push_back(printer.get());
          }
        }
        break;
      case ExternalPrinters::BLACKLIST_ONLY:
        for (const auto& printer : *cache) {
          if (!base::ContainsKey(blacklist_, printer->id())) {
            view->push_back(printer.get());
          }
        }
        break;
      case ExternalPrinters::ALL_ACCESS:
        for (const auto& printer : *cache) {
          view->push_back(printer.get());
        }
        break;
    }

    return view;
  }

 private:
  std::set<std::string> blacklist_;
  std::set<std::string> whitelist_;
  SEQUENCE_CHECKER(sequence_checker_);
  DISALLOW_COPY_AND_ASSIGN(Restrictions);
};

class ExternalPrintersImpl : public ExternalPrinters {
 public:
  ExternalPrintersImpl()
      : restrictions_(base::MakeUnique<Restrictions>()),
        restrictions_runner_(base::CreateSequencedTaskRunnerWithTraits(
            {base::TaskPriority::USER_VISIBLE,
             base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})),
        printers_(std::make_unique<PrinterView>()),
        weak_ptr_factory_(this) {}
  ~ExternalPrintersImpl() override {
    restrictions_runner_->DeleteSoon(FROM_HERE, std::move(restrictions_));
  }

  // Resets the printer state fields.
  void ClearData() override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    if (!base::FeatureList::IsEnabled(features::kBulkPrinters)) {
      return;
    }

    // Notify if we're transfering from a non-empty state to an empty state.
    bool notify = (printers_ != nullptr) ? !printers_->empty() : false;
    received_data_ = false;

    // Reallocate with default objects.
    printers_cache_ = base::MakeUnique<PrinterCache>();
    printers_ = base::MakeUnique<PrinterView>();

    if (notify) {
      Notify();
    }
  }

  void SetData(std::unique_ptr<std::string> data) override {
    if (!base::FeatureList::IsEnabled(features::kBulkPrinters)) {
      return;
    }

    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    if (!data) {
      LOG(WARNING) << "Received null data";
      return;
    }

    // Invalidate the printers cache and the view.  They will be recomputed.
    pending_parsing_requests_++;
    received_data_ = true;
    printers_cache_ = nullptr;
    printers_ = nullptr;
    base::PostTaskAndReplyWithResult(
        FROM_HERE, base::BindOnce(&ParsePrinters, std::move(data)),
        base::BindOnce(&ExternalPrintersImpl::OnParsingComplete,
                       weak_ptr_factory_.GetWeakPtr()));
  }

  void OnParsingComplete(std::unique_ptr<PrinterCache> parsed_cache) {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    pending_parsing_requests_--;
    if (pending_parsing_requests_ > 0) {
      // Overlapping requests are processing.  Wait for the last one.
      return;
    }

    if (!parsed_cache) {
      // Parsing failed.  Set to an empty cache so we know we've completed
      // parsing.
      printers_cache_ = base::MakeUnique<PrinterCache>();
    } else {
      printers_cache_ = std::move(parsed_cache);
    }

    MaybeRecomputePrinters();
  }

  void AddObserver(Observer* observer) override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    observers_.AddObserver(observer);
  }

  void RemoveObserver(Observer* observer) override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    observers_.RemoveObserver(observer);
  }

  void SetAccessMode(AccessMode mode) override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    mode_ = mode;
    MaybeRecomputePrinters();
  }

  void SetBlacklist(const std::vector<std::string>& blacklist) override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    has_blacklist_ = true;
    restrictions_runner_->PostTaskAndReply(
        FROM_HERE,
        base::BindOnce(&Restrictions::SetBlacklist,
                       base::Unretained(restrictions_.get()), blacklist),
        base::BindOnce(&ExternalPrintersImpl::MaybeRecomputePrinters,
                       weak_ptr_factory_.GetWeakPtr()));
  }

  void SetWhitelist(const std::vector<std::string>& whitelist) override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    has_whitelist_ = true;
    restrictions_runner_->PostTaskAndReply(
        FROM_HERE,
        base::BindOnce(&Restrictions::SetWhitelist,
                       base::Unretained(restrictions_.get()), whitelist),
        base::BindOnce(&ExternalPrintersImpl::MaybeRecomputePrinters,
                       weak_ptr_factory_.GetWeakPtr()));
  }

  bool IsPolicySet() const override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    return IsReady() && received_data_;
  }

  const std::vector<const Printer*>* GetPrinters() const override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    return printers_.get();
  }

 private:
  // Returns true if the necessary restrictions have been set to compute the
  // list of printers.
  bool IsReady() const {
    switch (mode_) {
      case AccessMode::ALL_ACCESS:
        return true;
      case AccessMode::BLACKLIST_ONLY:
        return has_blacklist_;
      case AccessMode::WHITELIST_ONLY:
        return has_whitelist_;
      case AccessMode::UNSET:
        return false;
    }
    NOTREACHED();
    return false;
  }

  // Notify all attached observervers.
  void Notify() {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    for (auto& observer : observers_) {
      // We rely on the assumption that this is sequenced with the rest of our
      // code.
      observer.OnPrintersChanged();
    }
  }

  // Called when |printers_| might require computation.  If we're not ready,
  // printers_ is cleared.
  void MaybeRecomputePrinters() {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

    // Something has changed. The view is now invalid.
    if (printers_) {
      printers_->clear();
    }

    if (!IsReady() || !printers_cache_) {
      // Do nothing unless we have a valid cache and restrictions.
      return;
    }

    // printers_ is nullptr during computation.
    printers_ = nullptr;
    // Give up ownership of the cache so we can compute the view.
    std::unique_ptr<PrinterCache> temp_list = std::move(printers_cache_);
    const PrinterCache* temp_list_ptr = temp_list.get();
    printers_cache_ = nullptr;

    base::PostTaskAndReplyWithResult(
        restrictions_runner_.get(), FROM_HERE,
        base::BindOnce(&Restrictions::ComputePrinters,
                       base::Unretained(restrictions_.get()), mode_,
                       base::Unretained(temp_list_ptr)),
        base::BindOnce(&ExternalPrintersImpl::OnComputationComplete,
                       weak_ptr_factory_.GetWeakPtr(), std::move(temp_list)));
  }

  // Called on computation completion.  |cache| is the cache with which the
  // computation was performed.  |view| is the computed printers which a user
  // should be able to see. |cache| and |view| are expected to never be null.
  void OnComputationComplete(std::unique_ptr<PrinterCache> cache,
                             std::unique_ptr<PrinterView> view) {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    DCHECK(cache);
    DCHECK(view);

    printers_cache_ = std::move(cache);
    printers_ = std::move(view);
    Notify();
  }

  // Holds the blacklist and whitelist.  Computes the effective printer list.
  std::unique_ptr<Restrictions> restrictions_;
  // Off UI sequence for computing the printer view.
  scoped_refptr<base::SequencedTaskRunner> restrictions_runner_;

  AccessMode mode_ = UNSET;
  bool has_blacklist_ = false;
  bool has_whitelist_ = false;

  // True if we've received a policy blob.
  bool received_data_ = false;
  // Number of parsing requests outstanding.  Parsed JSON results are ignored if
  // there are other results pending.
  int pending_parsing_requests_ = 0;
  // Cache of the parsed printer configuration file.
  std::unique_ptr<PrinterCache> printers_cache_;
  // The computed set of printers.  Printers are owned by all_printers_.
  std::unique_ptr<PrinterView> printers_;

  base::ObserverList<ExternalPrinters::Observer> observers_;

  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<ExternalPrintersImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ExternalPrintersImpl);
};

}  // namespace

// static
std::unique_ptr<ExternalPrinters> ExternalPrinters::Create() {
  return base::MakeUnique<ExternalPrintersImpl>();
}

}  // namespace chromeos
