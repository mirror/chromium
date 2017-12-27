// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/ios/browser/autofill_switches.h"
#include "build/build_config.h"

namespace autofill {
namespace switches {

// Removes the requirement that we recieved a ping from the autofill servers
// and that the user doesn't have the given form blacklisted. Used in testing.
const char kAutofillDelayBetweenFields[] = "autofill-ios-delay-between-fields";

}  // namespace switches
}  // namespace autofill
