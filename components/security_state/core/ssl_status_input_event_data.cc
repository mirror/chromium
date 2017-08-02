// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/security_state/core/ssl_status_input_event_data.h"

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"

namespace security_state {

// TODO(elawrence): Migrate displayed_password_field_on_http and
// displayed_credit_card_field_on_http to the UserData object too.
SSLStatusInputEventData::SSLStatusInputEventData() {}
SSLStatusInputEventData::~SSLStatusInputEventData() {}

void SSLStatusInputEventData::set_insecure_field_edited(
    bool insecure_field_edited) {
  insecure_field_edited_ = insecure_field_edited;
}
bool SSLStatusInputEventData::insecure_field_edited() {
  return insecure_field_edited_;
}

// SSLStatus implementation:
std::unique_ptr<SSLStatus::UserData> SSLStatusInputEventData::Clone() {
  std::unique_ptr<SSLStatusInputEventData> cloned =
      base::MakeUnique<SSLStatusInputEventData>();
  cloned->set_insecure_field_edited(insecure_field_edited_);
  return std::move(cloned);
}

}  // namespace security_state
