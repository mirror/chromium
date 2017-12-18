// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/content/common/submission_source_util.h"

namespace autofill {

SubmissionSource FromMojomSubmissionSource(
    mojom::AutofillDriver::SubmissionSource source) {
  switch (source) {
    case mojom::AutofillDriver::SubmissionSource::SAME_DOCUMENT_NAVIGATION:
      return SAME_DOCUMENT_NAVIGATION;
    case mojom::AutofillDriver::SubmissionSource::XHR_SUCCEEDED:
      return XHR_SUCCEEDED;
    case mojom::AutofillDriver::SubmissionSource::FRAME_DETACHED:
      return FRAME_DETACHED;
    case mojom::AutofillDriver::SubmissionSource::DOM_MUTATION_AFTER_XHR:
      return DOM_MUTATION_AFTER_XHR;
    case mojom::AutofillDriver::SubmissionSource::PROBABLY_FORM_SUBMITTED:
      return PROBABLY_FORM_SUBMITTED;
    case mojom::AutofillDriver::SubmissionSource::FORM_SUBMISSION:
      return FORM_SUBMISSION;
    case mojom::AutofillDriver::SubmissionSource::LAST:
      NOTREACHED();
      return LAST;
  }
  NOTREACHED();
  return LAST;
}

mojom::AutofillDriver::SubmissionSource ToMojomSubmissionSource(
    SubmissionSource source) {
  switch (source) {
    case SAME_DOCUMENT_NAVIGATION:
      return mojom::AutofillDriver::SubmissionSource::SAME_DOCUMENT_NAVIGATION;
    case XHR_SUCCEEDED:
      return mojom::AutofillDriver::SubmissionSource::XHR_SUCCEEDED;
    case FRAME_DETACHED:
      return mojom::AutofillDriver::SubmissionSource::FRAME_DETACHED;
    case DOM_MUTATION_AFTER_XHR:
      return mojom::AutofillDriver::SubmissionSource::DOM_MUTATION_AFTER_XHR;
    case PROBABLY_FORM_SUBMITTED:
      return mojom::AutofillDriver::SubmissionSource::PROBABLY_FORM_SUBMITTED;
    case FORM_SUBMISSION:
      return mojom::AutofillDriver::SubmissionSource::FORM_SUBMISSION;
    case LAST:
      NOTREACHED();
      return mojom::AutofillDriver::SubmissionSource::LAST;
  }
  NOTREACHED();
  return mojom::AutofillDriver::SubmissionSource::LAST;
}

}  // namespace autofill
