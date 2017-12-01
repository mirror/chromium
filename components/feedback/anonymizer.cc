// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/feedback/anonymizer.h"

namespace feedback {

Anonymizer::Anonymizer() : anonymizer_(new AnonymizerTool) {}

Anonymizer::~Anonymizer() {}

void Anonymizer::Anonymize(std::unique_ptr<SystemLogsResponse> input,
                           const SysLogsSourceCallback& callback) {
  for (auto& element : *input) {
    element.second = anonymizer_->Anonymize(element.second);
  }
  callback.Run(std::move(input));
}

}  // namespace feedback
