// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_COMMON_SUBMISSION_SOURCE_H_
#define COMPONENTS_AUTOFILL_CORE_COMMON_SUBMISSION_SOURCE_H_

namespace autofill {

enum SubmissionSource {
  SAME_DOCUMENT_NAVIGATION,
  XHR_SUCCEEDED,
  FRAME_DETACHED,
  DOM_MUTATION_AFTER_XHR,
  PROBABLY_FORM_SUBMITTED,
  FORM_SUBMISSION,
  LAST,
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_COMMON_FORM_SUBMISSION_SOURCE_H_
