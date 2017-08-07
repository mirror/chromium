// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_COMPONENT_EXTENSION_DELEGATE_H_
#define EXTENSIONS_BROWSER_COMPONENT_EXTENSION_DELEGATE_H_

namespace base {
class FilePath;
}

namespace extensions {

// This class manages interactions with component extensions and checks whether
// a resource should come from the component extension resource bundle.
class ComponentExtensionDelegate {
 public:
  virtual ~ComponentExtensionDelegate() {}

  // Checks whether image is a component extension resource. Returns false
  // if a given |resource| does not have a corresponding image in bundled
  // resources. Otherwise fills |resource_id|. This doesn't check if the
  // extension the resource is in is actually a component extension.
  virtual bool IsComponentExtensionResource(
      const base::FilePath& extension_path,
      const base::FilePath& resource_path,
      int* resource_id) const = 0;
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_COMPONENT_EXTENSION_DELEGATE_H_
