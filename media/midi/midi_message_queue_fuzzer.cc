// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include "media/midi/midi_message_queue.h"

namespace {

std::unique_ptr<midi::MidiMessageQueue> sQueue;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (!sQueue) {
    sQueue = std::make_unique<midi::MidiMessageQueue>(true);
  }

  sQueue->Add(data, size);

  std::vector<uint8_t> message;
  while (true) {
    sQueue->Get(&message);
    if (message.empty()) {
      break;
    }
  }

  return 0;
}
