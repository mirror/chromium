// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ResourceLoader_h
#define ResourceLoader_h

#include "platform/PlatformExport.h"
//#include "platform/wtf/text/WTFString.h"
#include "public/platform/WebString.h"

namespace blink {

// Provides access to ui::ResourceBundle in Blink. This
// allows Blink to directly load resources from the bundle.
class ResourceLoader {
 public:
  // Returns the contents of a resource as a string specified by the
  // resource id from Grit.
  static PLATFORM_EXPORT WebString GetResourceAsString(int resource_id);

  // Uncompresses a gzipped resource and returns it as a string. The resource
  // is specified by the resource id from Grit.
  static PLATFORM_EXPORT WebString UncompressResourceAsString(int resource_id);
};

}  // namespace blink

#endif  // ResourceLoader_h
