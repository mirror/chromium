// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/ptr_util.h"
#include "base/trace_event/trace_event.h"
#include "base/trace_event/trace_event_argument.h"

using base::trace_event::TracedValue;


int main() {
  uintptr_t dont_outsmart_this_test = 0;
  auto value = base::MakeUnique<TracedValue>();
  value->BeginArray("nodes");
  for (int i = 0; i < 100000; i++) {
    value->BeginDictionary();
    value->SetString("value", "foobbarrrrrrrr");
    value->EndDictionary();
  }
  value->EndArray();
  dont_outsmart_this_test += (uintptr_t) value.get();
  return (int) dont_outsmart_this_test;
}
