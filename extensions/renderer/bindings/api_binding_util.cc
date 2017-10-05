// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/bindings/api_binding_util.h"

#include "base/logging.h"
#include "base/supports_user_data.h"
#include "build/build_config.h"
#include "gin/per_context_data.h"

namespace extensions {
namespace binding {

namespace {

const char kInvalidatedContextFlagKey[] = "extension_invalidated_context";
// An empty struct that just serves as a flag for invalidated contexts. If the
// context has an instance of this struct, it has been invalidated.
struct InvalidatedContextFlag : public base::SupportsUserData::Data {
  // This space intentionally left blank.
};

}  // namespace

bool IsContextValid(v8::Local<v8::Context> context) {
  gin::PerContextData* per_context_data = gin::PerContextData::From(context);
  if (!per_context_data)
    return false;
  auto* data = static_cast<InvalidatedContextFlag*>(
      per_context_data->GetUserData(kInvalidatedContextFlagKey));
  return !data;
}

void InvalidateContext(v8::Local<v8::Context> context) {
  gin::PerContextData* per_context_data = gin::PerContextData::From(context);
  if (!per_context_data)
    return;  // Context is already invalidated at a higher level.
  per_context_data->SetUserData(kInvalidatedContextFlagKey,
                                std::make_unique<InvalidatedContextFlag>());
}

std::string GetPlatformString() {
#if defined(OS_CHROMEOS)
  return "chromeos";
#elif defined(OS_LINUX)
  return "linux";
#elif defined(OS_MACOSX)
  return "mac";
#elif defined(OS_WIN)
  return "win";
#else
  NOTREACHED();
  return std::string();
#endif
}

}  // namespace binding
}  // namespace extensions
