// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/message_target.h"

namespace extensions {

MessageTarget MessageTarget::ForTab(int tab_id, int frame_id) {
  return MessageTarget(tab_id, frame_id);
}

MessageTarget MessageTarget::ForExtension(const std::string& extension_id) {
  return MessageTarget(EXTENSION, extension_id);
}

MessageTarget MessageTarget::ForNativeApp(const std::string& native_app) {
  return MessageTarget(NATIVE_APP, native_app);
}

MessageTarget::MessageTarget(MessageTarget&& other) = default;
MessageTarget::MessageTarget(const MessageTarget& other) = default;
MessageTarget::~MessageTarget() {}

bool MessageTarget::operator==(const MessageTarget& other) const {
  return type == other.type && extension_id == other.extension_id &&
         native_application_name == other.native_application_name &&
         tab_id == other.tab_id && frame_id == other.frame_id;
}

MessageTarget::MessageTarget(int tab_id, int frame_id)
    : type(TAB), tab_id(tab_id), frame_id(frame_id) {}
MessageTarget::MessageTarget(Type type,
                             const std::string& extension_id_or_app_name)
    : type(type) {
  if (type == EXTENSION) {
    extension_id = extension_id_or_app_name;
  } else {
    DCHECK_EQ(NATIVE_APP, type);
    native_application_name = extension_id_or_app_name;
  }
}

}  // namespace extensions
