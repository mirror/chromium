// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SECURITY_STATE_CORE_SSL_STATUS_INPUT_EVENT_DATA_H_
#define COMPONENTS_SECURITY_STATE_CORE_SSL_STATUS_INPUT_EVENT_DATA_H_

#include <memory>

#include "content/public/browser/ssl_status.h"

namespace security_state {

using content::SSLStatus;

// TODO(elawrence): Migrate displayed_password_field_on_http and
// displayed_credit_card_field_on_http to the UserData object too.

// Stores flags about Input Events in the SSLStatus UserData to
// influence the Security Level of the page.
class SSLStatusInputEventData : public SSLStatus::UserData {
 public:
  SSLStatusInputEventData();
  ~SSLStatusInputEventData() override;

  void set_insecure_field_edited(bool insecure_field_edited);
  bool insecure_field_edited();

  // SSLStatus implementation:
  std::unique_ptr<SSLStatus::UserData> Clone() override;

 private:
  bool insecure_field_edited_ = false;
  DISALLOW_COPY_AND_ASSIGN(SSLStatusInputEventData);
};

}  // namespace security_state

#endif  // COMPONENTS_SECURITY_STATE_CORE_SSL_STATUS_INPUT_EVENT_DATA_H_
