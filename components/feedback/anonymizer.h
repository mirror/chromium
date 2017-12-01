// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_FEEDBACK_ANONYMIZER_H_
#define COMPONENTS_FEEDBACK_ANONYMIZER_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "components/feedback/anonymizer_tool.h"
#include "components/feedback/feedback_common.h"

namespace feedback {

using SystemLogsResponse = FeedbackCommon::SystemLogsMap;

// Callback that the data sources use to return data.
using SysLogsSourceCallback =
    base::Callback<void(std::unique_ptr<SystemLogsResponse>)>;

class Anonymizer {
 public:
  Anonymizer();
  ~Anonymizer();

  void Anonymize(std::unique_ptr<SystemLogsResponse> map,
                 const SysLogsSourceCallback& callback);

 private:
  std::unique_ptr<AnonymizerTool> anonymizer_;

  DISALLOW_COPY_AND_ASSIGN(Anonymizer);
};

}  // namespace feedback

#endif  // COMPONENTS_FEEDBACK_ANONYMIZER_H_
