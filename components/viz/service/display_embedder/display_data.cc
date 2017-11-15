// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/display_embedder/display_data.h"

#include "components/viz/common/frame_sinks/begin_frame_source.h"
#include "components/viz/service/display/display.h"

namespace viz {

DisplayData::DisplayData() = default;

DisplayData::DisplayData(DisplayData&& other) = default;

DisplayData::~DisplayData() = default;

DisplayData& DisplayData::operator=(DisplayData&& other) = default;

}  // namespace viz
