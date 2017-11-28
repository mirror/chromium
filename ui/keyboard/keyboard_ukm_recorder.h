// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_KEYBOARD_KEYBOARD_UKM_RECORDER_H_
#define UI_KEYBOARD_KEYBOARD_UKM_RECORDER_H_

#include <string>

#include "base/macros.h"
#include "services/metrics/public/cpp/ukm_source_id.h"
#include "ui/base/ime/text_input_type.h"
#include "ui/keyboard/keyboard_export.h"
#include "url/gurl.h"

namespace keyboard {

class KEYBOARD_EXPORT KeyboardUkmRecorder {
 public:
  KeyboardUkmRecorder() = default;

  void RecordUkm(const GURL& source_url, const ui::TextInputType input_type);

 private:
  ukm::SourceId ukm_source_id_ = ukm::kInvalidSourceId;
  GURL last_recorded_url_ = GURL::EmptyGURL();

  DISALLOW_COPY_AND_ASSIGN(KeyboardUkmRecorder);
};

}  // namespace keyboard

#endif  // UI_KEYBOARD_KEYBOARD_UKM_RECORDER_H_
