// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebUKMFeatures_h
#define WebUKMFeatures_h

#include "third_party/WebKit/Source/core/frame/WebFeature.h"

// This file defines a list of UseCounter WebFeature_s being measured in the
// UKM-based UseCounter. Features must all satisfy UKM privacy requirements
// (see go/ukm-logs). Features should only be added if it's shown (or highly
// likely be) rare, e.g. <1% of page views as measured by UMA.
namespace blink {
std::set<WebFeature> WebUKMFeatures = {};
}  // namespace blink

#endif  // WebUKMFeatures_h
