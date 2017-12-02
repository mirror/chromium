// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/printing/external_printers_factory.h"

#include <memory>

#include "base/lazy_instance.h"
#include "chrome/browser/chromeos/printing/external_printers.h"

namespace chromeos {

namespace {

base::LazyInstance<ExternalPrintersFactory>::DestructorAtExit
    g_printers_factory = LAZY_INSTANCE_INITIALIZER;

}  // namespace

// static
ExternalPrintersFactory* ExternalPrintersFactory::Get() {
  return g_printers_factory.Pointer();
}

ExternalPrinters* ExternalPrintersFactory::GetForAccountId(
    const AccountId& account_id) {
  auto found = printers_by_user_.find(account_id);
  if (found != printers_by_user_.end()) {
    return found->second.get();
  }

  printers_by_user_[account_id] = ExternalPrinters::Create();
  return printers_by_user_[account_id].get();
}

void ExternalPrintersFactory::RemoveForUserId(const AccountId& account_id) {
  printers_by_user_.erase(account_id);
}

void ExternalPrintersFactory::Shutdown() {
  printers_by_user_.clear();
}

ExternalPrintersFactory::ExternalPrintersFactory() = default;
ExternalPrintersFactory::~ExternalPrintersFactory() = default;

}  // namespace chromeos
