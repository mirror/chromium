// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_FORM_UKM_RECORDER_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_FORM_UKM_RECORDER_H_

#include <memory>

#include "base/macros.h"
#include "components/password_manager/core/browser/password_form_manager.h"
#include "components/ukm/public/ukm_recorder.h"

namespace password_manager {

// A recorder for UKM Entries that records metrics and reports them in one
// UKM Entry on destruction. All metrics recorded during the life-cycle of this
// instance are guaranteed to be grouped in a single UKM Entry.
class PasswordFormUkmRecorder {
 public:
  PasswordFormUkmRecorder(ukm::UkmRecorder* ukm_recorder,
                          ukm::SourceId source_id);
  ~PasswordFormUkmRecorder();

  ukm::UkmEntryBuilder& get_builder() { return *entry_builder_; }

 private:
  // This builder collects the metrics to report and reports them on
  // destruction.
  std::unique_ptr<ukm::UkmEntryBuilder> entry_builder_;

  DISALLOW_COPY_AND_ASSIGN(PasswordFormUkmRecorder);
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_FORM_UKM_RECORDER_H_
