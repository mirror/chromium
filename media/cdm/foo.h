// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CDM_FOO_H_
#define MEDIA_CDM_FOO_H_

#include <memory>
#include <vector>

#include "media/base/media_export.h"

namespace media {

class MEDIA_EXPORT Foo {
 private:
  std::vector<std::unique_ptr<int>> i;
};

}  // namespace media

#endif  // MEDIA_CDM_FOO_H_
