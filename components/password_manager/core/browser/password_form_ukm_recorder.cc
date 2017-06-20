// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/password_form_ukm_recorder.h"

#include "components/ukm/public/ukm_entry_builder.h"

namespace {
constexpr const char kPasswordFormUkmEntry[] = "PasswordForm";
}

namespace password_manager {

PasswordFormUkmRecorder::PasswordFormUkmRecorder(ukm::UkmRecorder* ukm_recorder,
                                                 ukm::SourceId source_id)
    : entry_builder_(
          ukm_recorder->GetEntryBuilder(source_id, kPasswordFormUkmEntry)) {}

PasswordFormUkmRecorder::~PasswordFormUkmRecorder() {
  // The implicit destruction of the entry_builder_ here triggers the reporting
  // of UKM metrics.
}

}  // namespace password_manager
