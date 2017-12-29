// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PRINTING_COMMON_PDF_METAFILE_UTILS_H_
#define PRINTING_COMMON_PDF_METAFILE_UTILS_H_

#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "printing/printing_export.h"
#include "skia/ext/platform_canvas.h"
#include "third_party/skia/include/core/SkDocument.h"
#include "third_party/skia/include/core/SkRefCnt.h"
#include "third_party/skia/include/core/SkSerialProcs.h"
#include "third_party/skia/include/core/SkStream.h"

namespace printing {

enum class SkiaDocumentType {
  PDF,
  // MSKP is an experimental, fragile, and diagnostic-only document type.
  MSKP,
  MAX = MSKP
};

// Stores content's global unique id and its SkPicture.
using DeserializationContext = base::flat_map<uint64_t, sk_sp<SkPicture>>;

// Stores current process id and a sequence of picture ids from printing
// out-of-process subframes.
struct SerializationContext {
  SerializationContext(uint32_t pid, const std::vector<uint32_t>& pic_ids)
      : process_id(pid), picture_ids(pic_ids) {}

  uint32_t process_id;
  const std::vector<uint32_t>& picture_ids;
};

sk_sp<SkDocument> MakePdfDocument(const std::string& creator,
                                  SkWStream* stream);

sk_sp<SkPicture> GetEmptyPicture();

SkSerialProcs SerializationProcs(SerializationContext* ctx);

SkDeserialProcs DeserializationProcs(DeserializationContext* ctx);

}  // namespace printing

#endif  // PRINTING_COMMON_PDF_METAFILE_UTILS_H_
