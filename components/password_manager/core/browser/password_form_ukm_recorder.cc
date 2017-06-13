// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/password_form_ukm_recorder.h"

#include "components/ukm/public/ukm_entry_builder.h"

namespace {
const char kPasswordFormUkmEntry[] = "PasswordForm";
}

namespace password_manager {

PasswordFormUkmRecorder::PasswordFormUkmRecorder(ukm::UkmRecorder* ukm_recorder,
                                                 ukm::SourceId source_id)
    : entry_builder_(
          ukm_recorder
              ? ukm_recorder->GetEntryBuilder(source_id, kPasswordFormUkmEntry)
              : nullptr) {}

PasswordFormUkmRecorder::~PasswordFormUkmRecorder() = default;

void PasswordFormUkmRecorder::RecordFormType(
    PasswordFormManager::FormType type) {
  if (!entry_builder_)
    return;
  entry_builder_->AddMetric("ClassificationResult", static_cast<int64_t>(type));
}

void PasswordFormUkmRecorder::RecordSubmitResult(
    PasswordFormManager::SubmitResult result) {
  switch (result) {
    case PasswordFormManager::kSubmitResultNotSubmitted:
      entry_builder_->AddMetric("Submission.Observed", 0);
      break;
    case PasswordFormManager::kSubmitResultFailed:
      entry_builder_->AddMetric("Submission.Observed", 1);
      entry_builder_->AddMetric("Submission.SubmissionResult", 2);
      break;
    case PasswordFormManager::kSubmitResultPassed:
      entry_builder_->AddMetric("Submission.Observed", 1);
      entry_builder_->AddMetric("Submission.SubmissionResult", 1);
      break;
    case PasswordFormManager::kSubmitResultMax:
      NOTREACHED();
  }
}

}  // namespace password_manager
