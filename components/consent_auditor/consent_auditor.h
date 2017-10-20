// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

namespace consent_auditor {

// Records that the user |profile| consented to a |feature|. The user was
// presented  with |description_text| and accepted it by interacting
// |confirmation_text| (e.g. clicking on a button; empty if not applicable).
// Returns true if successful.
bool RecordLocalConsent(Profile* profile,
                        const std::string& feature,
                        const std::string& description_text,
                        const std::string& confirmation_text);

}  // namespace consent_auditor
