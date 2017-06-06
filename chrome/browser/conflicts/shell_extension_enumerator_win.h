// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CONFLICTS_SHELL_EXTENSION_ENUMERATOR_WIN_H_
#define CHROME_BROWSER_CONFLICTS_SHELL_EXTENSION_ENUMERATOR_WIN_H_

#include <vector>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"

// Finds shell extensions installed on the computer by enumerating the registry.
class ShellExtensionEnumerator {
 public:
  // The path to the registry key where shell extensions are registered.
  static const wchar_t kShellExtensionRegistryKey[];
  static const wchar_t kClassIdRegistryKeyFormat[];

  using OnShellExtensionEnumeratedCallback =
      base::Callback<void(const base::FilePath&, uint32_t, uint32_t)>;

  explicit ShellExtensionEnumerator(const OnShellExtensionEnumeratedCallback&
                                        on_shell_extension_enumerated_callback);
  ~ShellExtensionEnumerator();

  // Returns the path of all the registered shell extensions. Must be called on
  // a blocking sequence.
  static std::vector<base::FilePath> GetShellExtensionPaths();

  // Reads the file on disk to find out the SizeOfImage and TimeDateStamp
  // properties of the module.
  static void GetModuleImageSizeAndTimeDateStamp(const base::FilePath& path,
                                                 uint32_t* size_of_image,
                                                 uint32_t* time_date_stamp);

 private:
  void OnShellExtensionEnumerated(const base::FilePath& path,
                                  uint32_t size_of_image,
                                  uint32_t time_date_stamp);

  OnShellExtensionEnumeratedCallback on_shell_extension_enumerated_callback_;

  base::WeakPtrFactory<ShellExtensionEnumerator> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ShellExtensionEnumerator);
};

#endif  // CHROME_BROWSER_CONFLICTS_SHELL_EXTENSION_ENUMERATOR_WIN_H_
