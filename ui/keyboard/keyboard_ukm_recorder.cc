// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/keyboard/keyboard_ukm_recorder.h"

#include "base/command_line.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "ui/base/ime/text_input_client.h"

namespace keyboard {
namespace {

bool IsUkmEnabled() {
  return base::FeatureList::IsEnabled(ukm::kUkmFeature);
}

}  // namespace

void KeyboardUkmRecorder::RecordUkm(const GURL& source_url,
                                    const ui::TextInputType input_type) {
  if (!IsUkmEnabled() || !source_url.is_valid())
    return;

  ukm::UkmRecorder* recorder = ukm::UkmRecorder::Get();
  if (last_recorded_url_ != source_url) {
    ukm_source_id_ = ukm::UkmRecorder::GetNewSourceID();
    recorder->UpdateSourceURL(ukm_source_id_, source_url);
    last_recorded_url_ = source_url;
  }
  ukm::builders::VirtualKeyboard_Open(ukm_source_id_)
      .SetTextInputType(input_type)
      .Record(recorder);
}

}  // namespace keyboard
