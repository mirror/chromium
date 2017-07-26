// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/loader/SubresourceIntegrityHelper.h"

#include "core/dom/ExecutionContext.h"
#include "core/frame/UseCounter.h"
#include "core/inspector/ConsoleMessage.h"
#include "core/inspector/ConsoleTypes.h"

namespace blink {

WebFeature GetWebFeature(
    SubresourceIntegrity::ReportInfo::UseCounterFeature& feature) {
  switch (feature) {
    case SubresourceIntegrity::ReportInfo::UseCounterFeature::
        kSRIElementWithMatchingIntegrityAttribute:
      return WebFeature::kSRIElementWithMatchingIntegrityAttribute;
    case SubresourceIntegrity::ReportInfo::UseCounterFeature::
        kSRIElementWithNonMatchingIntegrityAttribute:
      return WebFeature::kSRIElementWithNonMatchingIntegrityAttribute;
    case SubresourceIntegrity::ReportInfo::UseCounterFeature::
        kSRIElementIntegrityAttributeButIneligible:
      return WebFeature::kSRIElementIntegrityAttributeButIneligible;
    case SubresourceIntegrity::ReportInfo::UseCounterFeature::
        kSRIElementWithUnparsableIntegrityAttribute:
      return WebFeature::kSRIElementWithUnparsableIntegrityAttribute;
  }
  NOTREACHED();
  return WebFeature::kSRIElementWithUnparsableIntegrityAttribute;
}

void SubresourceIntegrityHelper::DoReport(
    ExecutionContext& execution_context,
    const SubresourceIntegrity::Result& result) {
  for (auto feature : result.GetReportInfo()->UseCounts()) {
    UseCounter::Count(&execution_context, GetWebFeature(feature));
  }
  for (const auto& message : result.GetReportInfo()->ConsoleErrorMessages()) {
    execution_context.AddConsoleMessage(ConsoleMessage::Create(
        kSecurityMessageSource, kErrorMessageLevel, message));
  }
}

}  // namespace blink
