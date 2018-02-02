// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/common/autofill_clock.h"

#include <utility>

#include "base/time/clock.h"
#include "base/time/default_clock.h"

namespace autofill {

AutofillClock AutofillClock::s_autofill_clock_;

// static
base::Time AutofillClock::Now() {
  if (!s_autofill_clock_.clock_)
    SetClock();

  return s_autofill_clock_.clock_->Now();
}

// static
void AutofillClock::SetClock() {
  s_autofill_clock_.clock_ = base::DefaultClock::GetInstance();
}

// static
void AutofillClock::SetTestClock(base::Clock* clock) {
  DCHECK(clock);
  s_autofill_clock_.clock_ = clock;
}

}  // namespace autofill
