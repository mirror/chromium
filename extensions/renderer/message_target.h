// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_RENDERER_MESSAGE_TARGET_H_
#define EXTENSIONS_RENDERER_MESSAGE_TARGET_H_

#include <string>

#include "base/optional.h"

namespace extensions {
struct Message;

struct MessageTarget {
  enum Type {
    TAB,
    EXTENSION,
    NATIVE_APP,
  };

  static MessageTarget ForTab(int tab_id, int frame_id);
  static MessageTarget ForExtension(const std::string& extension_id);
  static MessageTarget ForNativeApp(const std::string& native_app_name);

  MessageTarget(MessageTarget&& other);
  MessageTarget(const MessageTarget& other);
  ~MessageTarget();

  Type type;
  // Only valid for Type::EXTENSION.
  base::Optional<std::string> extension_id;
  // Only valid for Type::NATIVE_APP.
  base::Optional<std::string> native_application_name;
  // Only valid for Type::TAB.
  base::Optional<int> tab_id;
  base::Optional<int> frame_id;

  bool operator==(const MessageTarget& other) const;

 private:
  MessageTarget(int tab_id, int frame_id);
  MessageTarget(Type type, const std::string& extension_id_or_app_name);
};

}  // namespace extensions

#endif  // EXTENSIONS_RENDERER_MESSAGE_TARGET_H_
