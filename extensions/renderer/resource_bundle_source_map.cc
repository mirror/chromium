// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/resource_bundle_source_map.h"

#include "base/logging.h"
#include "base/strings/string_piece.h"
#include "extensions/renderer/static_v8_external_one_byte_string_resource.h"
#include "extensions/renderer/v8_helpers.h"
#include "third_party/zlib/google/compression_utils.h"
#include "ui/base/resource/resource_bundle.h"

namespace extensions {

namespace {

v8::Local<v8::String> ConvertExternalString(v8::Isolate* isolate,
                                            const base::StringPiece& string) {
  // v8 takes ownership of the StaticV8ExternalOneByteStringResource (see
  // v8::String::NewExternal()).
  return v8::String::NewExternal(
      isolate, new StaticV8ExternalOneByteStringResource(string));
}

}  // namespace

ResourceBundleSourceMap::ResourceBundleSourceMap(
    const ui::ResourceBundle* resource_bundle)
    : resource_bundle_(resource_bundle) {
}

ResourceBundleSourceMap::~ResourceBundleSourceMap() {
}

void ResourceBundleSourceMap::RegisterSource(const char* const name,
                                             int resource_id,
                                             bool gzipped) {
  resource_map_[name] = {resource_id, gzipped};
}

v8::Local<v8::String> ResourceBundleSourceMap::GetSource(
    v8::Isolate* isolate,
    const std::string& name) const {
  const auto& resource_iter = resource_map_.find(name);
  if (resource_iter == resource_map_.end()) {
    NOTREACHED() << "No module is registered with name \"" << name << "\"";
    return v8::Local<v8::String>();
  }
  base::StringPiece resource =
      resource_bundle_->GetRawDataResource(resource_iter->second.id);
  if (resource.empty()) {
    NOTREACHED()
        << "Module resource registered as \"" << name << "\" not found";
    return v8::Local<v8::String>();
  }

  if (!resource_iter->second.gzipped)
    return ConvertExternalString(isolate, resource);

  std::string uncompressed_storage;
  uint32_t size = compression::GetUncompressedSize(resource);
  uncompressed_storage.resize(size);
  base::StringPiece uncompressed(uncompressed_storage.c_str(), size);
  if (!compression::GzipUncompress(resource, uncompressed))
    return v8::Local<v8::String>();

  v8::Local<v8::String> output;
  if (!v8_helpers::ToV8String(isolate, uncompressed_storage, &output))
    return v8::Local<v8::String>();
  return output;
}

bool ResourceBundleSourceMap::Contains(const std::string& name) const {
  return !!resource_map_.count(name);
}

}  // namespace extensions
