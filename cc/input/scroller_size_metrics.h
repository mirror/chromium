// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_INPUT_SCROLLER_SIZE_METRICS_H_
#define CC_INPUT_SCROLLER_SIZE_METRICS_H_

namespace cc {

// Use the two constants to add scroller size related UMA metrics.
// The goal of the metrics is to find the frequency of scrollers of
// different sizes that get scrolled. This experiment is aiming at small
// scrollers therefore the big ones, e.g. larger than 400px * 500px, will get
// capped into one bucket.
static constexpr int kScrollerSizeLargestBucket = 200000;
static constexpr int kScrollerSizeBucketCount = 50;
// By setting the threshold to 50000 (250 * 200) we might affect roughly 50%
// scrollers for memory saving. Meanwhile at most 4% scroll may be slowed down.
static constexpr int kSmallScrollerThreshold = 50000;

}  // namespace cc

#endif  // CC_INPUT_SCROLLER_SIZE_METRICS_H_
