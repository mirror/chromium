// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CONTENT_COMMON_SUBMISSION_SOURCE_TYPE_CONVERTERS_H_
#define COMPONENTS_AUTOFILL_CONTENT_COMMON_SUBMISSION_SOURCE_TYPE_CONVERTERS_H_

#include "components/autofill/content/common/autofill_driver.mojom.h"
#include "components/autofill/core/common/submission_source.h"

namespace mojo {

template <>
struct TypeConverter<autofill::mojom::AutofillDriver::SubmissionSource,
                     autofill::SubmissionSource> {
  static autofill::mojom::AutofillDriver::SubmissionSource Convert(
      autofill::SubmissionSource source);
};

template <>
struct TypeConverter<autofill::SubmissionSource,
                     autofill::mojom::AutofillDriver::SubmissionSource> {
  static autofill::SubmissionSource Convert(
      autofill::mojom::AutofillDriver::SubmissionSource source);
};

}  // namespace mojo

#endif  // COMPONENTS_AUTOFILL_CONTENT_COMMON_SUBMISSION_SOURCE_TYPE_CONVERTERS_H_
