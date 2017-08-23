// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/ResourceLoader.h"

#include "public/platform/WebString.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/resource/resource_handle.h"

namespace blink {

String ResourceLoader::GetResourceAsString(int resource_id) {
  ui::ResourceBundle& bundle = ui::ResourceBundle::GetSharedInstance();
  return WebString::FromUTF8(
      bundle.GetRawDataResourceForScale(resource_id, bundle.GetMaxScaleFactor())
          .as_string());
};

}  // namespace blink
