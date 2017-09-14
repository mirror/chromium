// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_EXTENSIONS_ZIP_ARCHIVER_PRIVATE_API_H_
#define CHROME_BROWSER_CHROMEOS_EXTENSIONS_ZIP_ARCHIVER_PRIVATE_API_H_

#include "base/compiler_specific.h"
#include "extensions/browser/extension_function.h"

class ZipArchiverPrivateGetLocalizedStringsFunction
    : public UIThreadExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("zipArchiverPrivate.getLocalizedStrings",
                             ZIPARCHIVERPRIVATE_GETLOCALIZEDSTRINGS)

 protected:
  ~ZipArchiverPrivateGetLocalizedStringsFunction() override {}

  // ExtensionFunction:
  ResponseAction Run() override;
};

#endif  // CHROME_BROWSER_CHROMEOS_EXTENSIONS_ZIP_ARCHIVER_PRIVATE_API_H_
