// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SECURITY_STATE_CORE_INSECURE_INPUT_EVENT_DATA_H_
#define COMPONENTS_SECURITY_STATE_CORE_INSECURE_INPUT_EVENT_DATA_H_

namespace security_state {

struct InsecureInputEventData {
  InsecureInputEventData() : insecure_field_edited(false) {}
  ~InsecureInputEventData() {}

  // True if the user edited a field on a non-secure page.
  bool insecure_field_edited;

  bool operator==(const InsecureInputEventData& other) const {
    return (insecure_field_edited == other.insecure_field_edited);
  }
};

}  // namespace security_state

#endif  // COMPONENTS_SECURITY_STATE_CORE_INSECURE_INPUT_EVENT_DATA_H_
