// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_CHROME_COMPONENT_EXTENSION_DELEGATE_H_
#define CHROME_BROWSER_EXTENSIONS_CHROME_COMPONENT_EXTENSION_DELEGATE_H_

#include <stddef.h>

#include <map>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "extensions/browser/component_extension_delegate.h"

struct GritResourceMap;

namespace extensions {

class ChromeComponentExtensionDelegate : public ComponentExtensionDelegate {
 public:
  ChromeComponentExtensionDelegate();
  ~ChromeComponentExtensionDelegate() override;

  // Overridden from ComponentExtensionDelegate:
  bool IsComponentExtensionResource(const base::FilePath& extension_path,
                                    const base::FilePath& resource_path,
                                    int* resource_id) const override;
  bool GetExtensionStrings(content::BrowserContext* browser_context,
                           const std::string& extension_id,
                           base::DictionaryValue*) const override;

 private:
  void AddComponentResourceEntries(const GritResourceMap* entries, size_t size);

  // A map from a resource path to the resource ID.  Used by
  // IsComponentExtensionResource.
  std::map<base::FilePath, int> path_to_resource_id_;

  DISALLOW_COPY_AND_ASSIGN(ChromeComponentExtensionDelegate);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_CHROME_COMPONENT_EXTENSION_DELEGATE_H_
