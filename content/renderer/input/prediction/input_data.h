// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_INPUT_PREDICTION_INPUT_DATA_H_
#define CONTENT_RENDERER_INPUT_PREDICTION_INPUT_DATA_H_

namespace input_prediction {

struct InputData {
  double pos_x;
  double pos_y;
  double pressure;
  double time_stamp_seconds;
};

}  // namespace input_prediction
#endif  // CONTENT_RENDERER_INPUT_PREDICTION_INPUT_DATA_H_
