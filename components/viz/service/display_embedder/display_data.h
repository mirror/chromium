// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_DISPLAY_DATA_H_
#define COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_DISPLAY_DATA_H_

#include <memory>

#include "base/macros.h"

namespace viz {

class Display;
class ExternalBeginFrameSource;
class SyntheticBeginFrameSource;

// Contains a Display and the BeginFrameSource for that display. Only one of
// |external_begin_frame_source| or |synthetic_begin_frame_source| will be
// non-null.
struct DisplayData {
  DisplayData();
  DisplayData(DisplayData&& other);
  ~DisplayData();
  DisplayData& operator=(DisplayData&& other);

  std::unique_ptr<ExternalBeginFrameSource> external_begin_frame_source;
  std::unique_ptr<SyntheticBeginFrameSource> synthetic_begin_frame_source;

  // Destroy |display| first.
  std::unique_ptr<Display> display;

 private:
  DISALLOW_COPY_AND_ASSIGN(DisplayData);
};

}  // namespace viz

#endif  //  COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_DISPLAY_DATA_H_
