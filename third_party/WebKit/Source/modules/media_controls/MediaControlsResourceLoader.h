// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MediaControlsResourceLoader_h
#define MediaControlsResourceLoader_h

#include "core/css/CSSDefaultStyleSheets.h"

namespace blink {

class MediaControlsResourceLoader
    : public CSSDefaultStyleSheets::UAStyleSheetLoader {
 public:
  static void InjectMediaControlsUAStyleSheet();

  String GetUAStyleSheet() override;

  String GetMediaControlsCSS();

  String GetMediaControlsMobileCSS();

 protected:
  MediaControlsResourceLoader();
};

}  // namespace blink

#endif  // MediaControlsResourceLoader_h
