// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PRINTING_COMMON_PDF_METAFILE_UTILS_H_
#define PRINTING_COMMON_PDF_METAFILE_UTILS_H_

#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "skia/ext/platform_canvas.h"
#include "third_party/skia/include/core/SkDocument.h"
#include "third_party/skia/include/core/SkRefCnt.h"
#include "third_party/skia/include/core/SkSerialProcs.h"
#include "third_party/skia/include/core/SkStream.h"

#define OopPicUniqueId(x, y) (((uint64_t)x) << 32 | y)

namespace printing {

enum class SkiaDocumentType {
  PDF,
  // MSKP is an experimental, fragile, and diagnostic-only document type.
  MSKP,
  MAX = MSKP
};

using DeserializationContext = base::flat_map<uint64_t, sk_sp<SkPicture>>;

struct SerializationContext {
  SerializationContext(uint32_t pid, const std::vector<uint32_t>& picture_ids)
      : process_id(pid), pic_ids(picture_ids) {}
  uint32_t process_id;
  const std::vector<uint32_t>& pic_ids;
};

sk_sp<SkDocument> MakePdfDocument(const std::string& creator,
                                  SkWStream* stream);

SkSerialProcs SerializationProcs(SerializationContext* ctx);

SkDeserialProcs DeserializationProcs(DeserializationContext* ctx);

}  // namespace printing

#endif  // PRINTING_COMMON_PDF_METAFILE_UTILS_H_
