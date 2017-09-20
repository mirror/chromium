// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_LAYERS_SQUASHING_RECORDING_SOURCE_H_
#define CC_LAYERS_SQUASHING_RECORDING_SOURCE_H_

#include <stddef.h>

#include <memory>

#include "cc/layers/recording_source.h"

namespace cc {

class CC_EXPORT SquashingRecordingSource : public RecordingSource {
 public:
  explicit SquashingRecordingSource(RecordingSource*);
  ~SquashingRecordingSource() override;

 private:
};
}  // namespace cc

#endif  // CC_LAYERS_SQUASHING_RECORDING_SOURCE_H_
