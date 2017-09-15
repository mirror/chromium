// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/media_controls/MediaControlsResourceLoader.h"

#include "blink/public/resources/grit/media_controls_resources.h"
#include "build/build_config.h"
#include "platform/ResourceLoader.h"
#include "platform/RuntimeEnabledFeatures.h"
#include "platform/wtf/text/WTFString.h"

namespace {

bool ShouldLoadAndroidCSS() {
#if defined(OS_ANDROID)
  return true;
#else
  return blink::RuntimeEnabledFeatures::MobileLayoutThemeEnabled();
#endif
}

}  // namespace.

namespace blink {

MediaControlsResourceLoader::MediaControlsResourceLoader()
    : UAStyleSheetLoader(){};

MediaControlsResourceLoader::~MediaControlsResourceLoader(){};

String MediaControlsResourceLoader::GetMediaControlsCSS() const {
  return ResourceLoader::UncompressResourceAsString(
      IDR_UASTYLE_MEDIA_CONTROLS_CSS);
};

String MediaControlsResourceLoader::GetMediaControlsAndroidCSS() const {
  return ResourceLoader::UncompressResourceAsString(
      IDR_UASTYLE_MEDIA_CONTROLS_ANDROID_CSS);
};

String MediaControlsResourceLoader::GetUAStyleSheet() {
  if (ShouldLoadAndroidCSS())
    return GetMediaControlsCSS() + GetMediaControlsAndroidCSS();
  return GetMediaControlsCSS();
}

void MediaControlsResourceLoader::InjectMediaControlsUAStyleSheet() {
  CSSDefaultStyleSheets& default_style_sheets =
      CSSDefaultStyleSheets::Instance();
  std::unique_ptr<MediaControlsResourceLoader> loader;

  if (!default_style_sheets.HasMediaControlsStyleSheetLoader())
    default_style_sheets.SetMediaControlsStyleSheetLoader(std::move(loader));
}

}  // namespace blink
