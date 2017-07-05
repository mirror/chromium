// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"
#include "ui/message_center/message_center.h"

// This file contains dummy implementation of message center and used to compile
// and link with the Android and Mac implementations which do not use the
// message center. This avoids spreading compile-time flags throughout the code.
#if !defined(OS_ANDROID) && !defined(OS_MACOSX)
#error This file should only be used on Android and Mac OS X.
#endif

namespace message_center {

// static
void MessageCenter::Initialize() {
}

// static
MessageCenter* MessageCenter::Get() {
  return NULL;
}

// static
void MessageCenter::Shutdown() {
}

MessageCenter::MessageCenter() {
}

MessageCenter::~MessageCenter() {
}

}  // namespace message_center
