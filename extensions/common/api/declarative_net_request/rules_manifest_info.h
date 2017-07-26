// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_COMMON_API_DECLARATIVE_NET_REQUEST_RULES_MANIFEST_INFO_H_
#define EXTENSIONS_COMMON_API_DECLARATIVE_NET_REQUEST_RULES_MANIFEST_INFO_H_

#include "base/macros.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_resource.h"
#include "extensions/common/manifest_handler.h"

namespace extensions {
namespace declarative_net_request {

// Manifest data required for the kDeclarativeNetRequestKey manifest
// key.
struct RulesManifestData : Extension::ManifestData {
  explicit RulesManifestData(const ExtensionResource& resource);
  ~RulesManifestData() override;

  // Returns ExtensionResource corresponding to the kDeclarativeNetRequestKey
  // manifest key for the |extension|. Returns nullptr if the extension didn't
  // specify the manifest key.
  static const ExtensionResource* GetRulesetResource(
      const Extension* extension);

  ExtensionResource resource;
  DISALLOW_COPY_AND_ASSIGN(RulesManifestData);
};

// Parses the kDeclarativeNetRequestKey manifest key.
class RulesManifestHandler : public ManifestHandler {
 public:
  RulesManifestHandler();
  ~RulesManifestHandler() override;

 private:
  bool Parse(Extension* extension, base::string16* error) override;
  bool Validate(const Extension* extension,
                std::string* error,
                std::vector<InstallWarning>* warnings) const override;
  const std::vector<std::string> Keys() const override;

  DISALLOW_COPY_AND_ASSIGN(RulesManifestHandler);
};

}  // namespace declarative_net_request
}  // namespace extensions

#endif  // EXTENSIONS_COMMON_API_DECLARATIVE_NET_REQUEST_RULES_MANIFEST_INFO_H_
