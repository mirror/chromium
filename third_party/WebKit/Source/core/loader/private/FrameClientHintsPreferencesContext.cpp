// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/loader/private/FrameClientHintsPreferencesContext.h"

#include "core/frame/UseCounter.h"

namespace blink {

namespace {

// Mapping from WebClientHintsType to WebFeatureMapping. The ordering of
// WebFeatures should match the ordering of enums in WebClientHintsType.
constexpr WebFeature WebFeatureMapping[kWebClientHintsTypeLast + 1] = {
    WebFeature::kClientHintsDeviceRAM, WebFeature::kClientHintsDPR,
    WebFeature::kClientHintsResourceWidth,
    WebFeature::kClientHintsViewportWidth,
};

}  // namespace

FrameClientHintsPreferencesContext::FrameClientHintsPreferencesContext(
    LocalFrame* frame)
    : frame_(frame) {}

void FrameClientHintsPreferencesContext::CountClientHints(
    WebClientHintsType type) {
  UseCounter::Count(frame_, WebFeatureMapping[static_cast<int32_t>(type)]);
}

}  // namespace blink
