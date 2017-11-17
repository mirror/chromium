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

KeyboardUkmRecorder::KeyboardUkmRecorder()
    : client_source_info_(""),
      text_input_type_(ui::TEXT_INPUT_TYPE_NONE),
      ukm_source_id_(-1) {}

void KeyboardUkmRecorder::UpdateUkmSource(const ui::TextInputClient* client) {
  if (!client || !IsUkmEnabled())
    return;

  const std::string new_source = client->GetClientSourceInfo();
  if (new_source.empty() || new_source == client_source_info_)
    return;

  client_source_info_ = new_source;
  text_input_type_ = client->GetTextInputType();
  GURL info_as_url(client_source_info_);
  if (info_as_url.is_valid()) {
    ukm_source_id_ = ukm::UkmRecorder::GetNewSourceID();
    ukm::UkmRecorder::Get()->UpdateSourceURL(ukm_source_id_, info_as_url);
  }
}

void KeyboardUkmRecorder::RecordUkm() {
  if (IsUkmEnabled() && GURL(client_source_info_).is_valid()) {
    ukm::builders::VirtualKeyboard_Open(ukm_source_id_)
        .SetTextInputType(text_input_type_)
        .Record(ukm::UkmRecorder::Get());
  }
}

}  // namespace keyboard
