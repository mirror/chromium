// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/keyboard/keyboard_ukm_recorder.h"

#include "services/metrics/public/cpp/ukm_builders.h"
#include "ui/base/ime/text_input_client.h"

namespace keyboard {

KeyboardUkmRecorder::KeyboardUkmRecorder()
    : client_source_info_(GURL::EmptyGURL()),
      text_input_type_(ui::TEXT_INPUT_TYPE_NONE),
      ukm_source_id_(-1) {}

void KeyboardUkmRecorder::UpdateUkmSource(const ui::TextInputClient* client) {
  if (!client)
    return;

  const GURL new_source = client->GetClientSourceInfo();
  if (new_source != client_source_info_ && !new_source.is_empty()) {
    client_source_info_ = new_source;
    text_input_type_ = client->GetTextInputType();
    ukm_source_id_ = ukm::UkmRecorder::GetNewSourceID();
    ukm::UkmRecorder::Get()->UpdateSourceURL(ukm_source_id_,
                                             client_source_info_);
  }
}

void KeyboardUkmRecorder::RecordUkm() {
  if (!client_source_info_.is_empty()) {
    ukm::builders::VirtualKeyboard_Open(ukm_source_id_)
        .SetTextInputType(text_input_type_)
        .Record(ukm::UkmRecorder::Get());
  }
}

}  // namespace keyboard
