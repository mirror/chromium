// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/subresource_filter/content/browser/console_messager.h"

#include "url/gurl.h"

namespace subresource_filter {

bool ConsoleMessager::ShouldLogSubresources() {
  return false;
}

}  // namespace subresource_filter
