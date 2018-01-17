// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/content/common/submission_source_type_converters.h"

namespace mojo {

using autofill::mojom::AutofillDriver;

// static
AutofillDriver::SubmissionSource TypeConverter<
    AutofillDriver::SubmissionSource,
    autofill::SubmissionSource>::Convert(autofill::SubmissionSource source) {
  switch (source) {
    case autofill::SAME_DOCUMENT_NAVIGATION:
      return AutofillDriver::SubmissionSource::SAME_DOCUMENT_NAVIGATION;
    case autofill::XHR_SUCCEEDED:
      return AutofillDriver::SubmissionSource::XHR_SUCCEEDED;
    case autofill::FRAME_DETACHED:
      return AutofillDriver::SubmissionSource::FRAME_DETACHED;
    case autofill::DOM_MUTATION_AFTER_XHR:
      return AutofillDriver::SubmissionSource::DOM_MUTATION_AFTER_XHR;
    case autofill::PROBABLY_FORM_SUBMITTED:
      return AutofillDriver::SubmissionSource::PROBABLY_FORM_SUBMITTED;
    case autofill::FORM_SUBMISSION:
      return AutofillDriver::SubmissionSource::FORM_SUBMISSION;
    case autofill::LAST:
      NOTREACHED();
      return AutofillDriver::SubmissionSource::FORM_SUBMISSION;
  }
  NOTREACHED();
  return AutofillDriver::SubmissionSource::FORM_SUBMISSION;
  ;
}

autofill::SubmissionSource
TypeConverter<autofill::SubmissionSource, AutofillDriver::SubmissionSource>::
    Convert(AutofillDriver::SubmissionSource source) {
  switch (source) {
    case AutofillDriver::SubmissionSource::SAME_DOCUMENT_NAVIGATION:
      return autofill::SAME_DOCUMENT_NAVIGATION;
    case AutofillDriver::SubmissionSource::XHR_SUCCEEDED:
      return autofill::XHR_SUCCEEDED;
    case AutofillDriver::SubmissionSource::FRAME_DETACHED:
      return autofill::FRAME_DETACHED;
    case AutofillDriver::SubmissionSource::DOM_MUTATION_AFTER_XHR:
      return autofill::DOM_MUTATION_AFTER_XHR;
    case AutofillDriver::SubmissionSource::PROBABLY_FORM_SUBMITTED:
      return autofill::PROBABLY_FORM_SUBMITTED;
    case AutofillDriver::SubmissionSource::FORM_SUBMISSION:
      return autofill::FORM_SUBMISSION;
  }
  NOTREACHED();
  return autofill::LAST;
}

}  // namespace mojo
