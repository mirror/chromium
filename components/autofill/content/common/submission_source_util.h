// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CONTENT_COMMON_SUBMISSION_SOURCE_UTIL_H_
#define COMPONENTS_AUTOFILL_CONTENT_COMMON_SUBMISSION_SOURCE_UTIL_H_

#include "components/autofill/content/common/autofill_driver.mojom.h"
#include "components/autofill/core/common/submission_source.h"

namespace autofill {

SubmissionSource FromMojomSubmissionSource(
    mojom::AutofillDriver::SubmissionSource source);

mojom::AutofillDriver::SubmissionSource ToMojomSubmissionSource(
    SubmissionSource source);

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CONTENT_COMMON_SUBMISSION_SOURCE_UTIL_H_
